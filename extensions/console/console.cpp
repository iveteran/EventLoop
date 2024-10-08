#include <errno.h>
#include <iostream>
#if defined(__linux__) && defined(SUPPORTS_READLINE)
    #include <readline/readline.h>
    #include <readline/history.h>
#endif
#include "console.h"
#include "eventloop/logger.h"

using namespace std;

namespace evt_loop
{

string to_lower(const string& str) {
    string lower_str;
    for (auto ch : str) {
        lower_str += tolower(ch);
    }
    return lower_str;
}

size_t get_max_size_of_words(const map<string, Console::CommandPtr>& cmds) {
    size_t max_size = 0;
    for (auto [cmd_name, _] : cmds) {
        if (cmd_name.size() > max_size) {
            max_size = cmd_name.size();
        }
    }
    return max_size;
}

#if defined(__linux__) && defined(SUPPORTS_READLINE)
void cb_line_handler(char *line) {
    if (line) {
        if (strlen(line) > 0) {
            add_history(line);
            Console::Instance()->handleInput(line);
        }
        free(line);
    } else {
        Console::Instance()->handleCtrl_D();
    }
}
#endif

Console *Console::instance_ = NULL;

void Console::init() {
#if defined(__linux__) && defined(SUPPORTS_READLINE)
    rl_callback_handler_install(prompt_.c_str(), cb_line_handler);
#endif
}

void Console::destory() {
#if defined(__linux__) && defined(SUPPORTS_READLINE)
    rl_callback_handler_remove();
#endif
}

void Console::OnEvents(uint32_t events) {
    if (events & FileEvent::READ) {
#if defined(__linux__) && defined(SUPPORTS_READLINE)
        rl_callback_read_char();
#else
        GetLine();
#endif
    }

    if (events & FileEvent::CLOSED) {
#if defined(__linux__) && defined(SUPPORTS_READLINE)
        rl_callback_handler_remove();
#endif
        OnClosed();
    } else if ((events & FileEvent::ERROR)) {
#if defined(__linux__) && defined(SUPPORTS_READLINE)
        rl_callback_handler_remove();
#endif
        OnError(errno, strerror(errno));
    }
}

size_t Console::GetLine() {
    string input;
    if (getline(cin, input)) {
        if (! input.empty()) {
            handleInput(input);
        }
    } else {
        handleCtrl_D();
    }
    os_ << prompt_ << flush;
    return input.size();
}

void Console::handleCtrl_D() {
    os_ << "Pressed Ctrl-D" << endl;
#if defined(__linux__) && defined(SUPPORTS_READLINE)
    //rl_callback_handler_remove();
#else
    cin.clear();
    clearerr(stdin);   // 重置输入流的状态
#endif
}

int Console::registerCommand(const char* cmd, const char* desc,
        const CommandCallback& cb, const ResultCallback& result_cb) {
    string errmsg = verifyCommand(cmd);
    if (! errmsg.empty()) {
        el_logger->error("[Console::registerCommand] Illegal command name: {}, reason: {}", cmd, errmsg);
        return -1;
    }
    string _cmd = to_lower(cmd);
    auto cmd_obj = std::make_shared<Command>(_cmd, desc, cb, result_cb);
    cmd_callbacks_.insert(std::make_pair(_cmd, cmd_obj));
    return 0;
}

string Console::verifyCommand(const string& cmd) {
    // Must be alphabet, number, '_' and '.', start with alphabet, '_' or '.'
    const char* errmsg = "the command must combined with alphabet, number, '_' and '.', and must start with alphabet, '_' or '.'";
    if (cmd.empty()) {
        return "the command must not be empty";
    } else if (cmd.size() > 32) {
        return "the command length must less than 32 characters";
    } else if (!(cmd[0] == '_' || cmd[0] == '.' || isalpha(cmd[0]))) {
        return errmsg;
    } else {
        for (auto ch : cmd) {
            if (!(ch == '_' || ch == '.' || isalpha(ch) || isdigit(ch))) {
                return errmsg;
            }
        }
    }
    return "";
}

void Console::handleInput(const string& input) {
    //put_line(input);
    auto argv = parseInput(input);
    if (! argv.empty()) {
        handleCommand(argv);
    }
}

vector<string> Console::parseInput(const string& input) {
    vector<string> words;
    string word;  // word or phrase
    for (size_t i=0; i<input.size(); i++) {
        char c = input[i];
        if (c == ' ') {
            if (! word.empty()) {
                words.push_back(word);
                word.clear();
            } else {
                continue;
            }
        } else if (c == '\"') {
            i++;
            for ( ; input[i] != '\"'; i++ ) {
                //if (input[i] != ' ' || (! word.empty() && word.back() != ' ')) {  // supports strip spaces at header
                if (input[i] != ' ' || word.back() != ' ') {
                    word.push_back(input[i]);
                }
            }
            // supports strip spaces at tail
            //if (word.back() == ' ') {
            //    word.erase(word.size()-1);
            //}
            words.push_back(word);
            word.clear();
        } else {
            word.push_back(c);
        }
    }
    if (! word.empty()) {
        words.push_back(word);
    }
    return words;
}

int Console::handleCommand(const vector<string>& argv) {
    auto cmd = to_lower(argv[0]);
    auto iter = cmd_callbacks_.find(cmd);
    if (iter == cmd_callbacks_.end()) {
        put_line("Invalid command, press .help to get help.");
        return -1;
    }
    auto cmd_obj = iter->second;
    return cmd_obj->callback(argv, cmd_obj->result_callback);
}

void Console::registerInnerCommand() {
    registerCommand(
            ".help",
            "print this help",
            std::bind(&Console::handleCommandHelp, this, std::placeholders::_1, std::placeholders::_2)
            );
    registerCommand(
            ".echo",
            "echo input, .echo [text]",
            std::bind(&Console::handleCommandEcho, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&Console::onCommandEchoResult, this, std::placeholders::_1, std::placeholders::_2)
            );
    registerCommand(
            ".change_prompt",
            "change console prompt, example: .change_prompt \"$ \"",
            std::bind(&Console::handleCommandChangePrompt, this, std::placeholders::_1, std::placeholders::_2)
            );
    registerCommand(
            ".log",
            "set log options",
            std::bind(&Console::handleCommandLog, this, std::placeholders::_1, std::placeholders::_2)
            );
    registerCommand(
            ".daemonize",
            "daemonize this process",
            std::bind(&Console::handleCommandDaemonize, this, std::placeholders::_1, std::placeholders::_2)
            );
}

int Console::handleCommandHelp(const vector<string>& argv, const ResultCallback& result_cb) {
    size_t max_size_of_cmd_name = get_max_size_of_words(cmd_callbacks_);
    auto twice_spaces = std::string(2, ' ');
    put_line("Commands: ");
    for (auto [cmd_name, cmd_obj] : cmd_callbacks_) {
        size_t space_times = max_size_of_cmd_name - cmd_name.size() + 2;
        auto some_spaces = std::string(space_times, ' ');
        put_line(twice_spaces, cmd_name, some_spaces, cmd_obj->desc);
    }
    return 0;
}

int Console::handleCommandEcho(const vector<string>& argv, const ResultCallback& result_cb) {
    for (size_t i=1; i<argv.size(); i++) {
        output(argv[i]);
        if (i < argv.size() - 1) {
            output(' ');
        }
    }
    output('\n');

    if (result_cb) {
        int status = 0;
        string result = "echo dummy result";
        result_cb(status, result);
    }
    return 0;
}
int Console::onCommandEchoResult(int status, const string& data) {
    put_line("* echo callback, status: ", status);
    put_line("* echo callback, data: ", data);
    return status;
}

int Console::handleCommandChangePrompt(const vector<string>& argv, const ResultCallback& result_cb) {
#if defined(SUPPORTS_READLINE)
    put_line("Unsupport in GNU readline mode");
    return 0;
#endif
    if (argv.size() > 1) {
        auto new_prompt = argv[1];
        if (prompt_ != new_prompt) {
            prompt_ = new_prompt;
        }
    } else {
        put_line("Invalid parameter");
    }
    return 0;
}

int Console::handleCommandLog(const vector<string>& argv, const ResultCallback& result_cb) {
    map<string, string> cmd_usages;
    cmd_usages["info"]   = ".log info             Show log info";
    cmd_usages["on"]     = ".log on               Enable log";
    cmd_usages["off"]    = ".log off              Disable log";
    cmd_usages["flush"]  = ".log flush            Flush log immediately";
    cmd_usages["color"]  = ".log color <on|off>   Enable or disable color for log level";
    cmd_usages["stdout"] = ".log stdout           Output log to STDOUT";
    cmd_usages["file"]   = ".log file <FILENAME>  Change log file to FILENAME";
    cmd_usages["help"]   = ".log help             Show this help";

    auto show_help = [this, cmd_usages]() {
        auto twice_spaces = std::string(2, ' ');
        put_line("Usage of command: log");
        for (auto [subcmd, usage] : cmd_usages) {
            put_line(twice_spaces, usage);
        }
    };
    auto is_logfilename_valid = [this](const string& filename) -> bool {
        // NOTE: the filename must is NOT a path, in other words it is a file at work directory of program
        return filename.find('/') == string::npos;  // Is be not includes '/'
    };

    if (argv.size() < 2) {
        put_line("Missing sub command.");
        show_help();
        return 1;
    }

    auto subcmd = argv[1];

    if (subcmd == "info") {
        auto info = el_logger->get_info();
        put_line(info);
    } else if (subcmd == "on") {
        if (! el_logger->is_disabled()) {
            put_line("The logger is already On");
        } else {
            el_logger->disable(false);
        }
    } else if (subcmd == "off") {
        if (el_logger->is_disabled()) {
            put_line("The logger is already Off");
        } else {
            el_logger->disable();
        }
    } else if (subcmd == "flush") {
        el_logger->flush();
    } else if (subcmd == "stdout") {
        el_logger->switch_to_stdout();
    } else if (subcmd == "file") {
        if (argv.size() > 2) {
            auto new_logfile = argv[2];
            if (is_logfilename_valid(new_logfile)) {
                el_logger->switch_to_file(new_logfile);
            } else {
                put_line("Invalid filename, the filename can not including path(with '/')");
            }
        } else {
            put_line("Invalid parameter.\nUsage: ", cmd_usages[subcmd]);
        }
    } else if (subcmd == "color") {
        if (argv.size() > 2 && (argv[2] == "on" || argv[2] == "off")) {
            bool on = argv[2] == "on" ? true : false;
            if (on == el_logger->is_colorful()) {
                put_line("The coloful is already ", (on ? "On" : "Off"));
            } else {
                el_logger->enable_colorful(on);
            }
        } else {
            put_line("Invalid parameter.\nUsage: ", cmd_usages[subcmd]);
        }
    } else if (subcmd == "help") {
        show_help();
    } else {
        put_line("Invalid sub command: ", subcmd);
        show_help();
    }
    return 0;
}

int Console::handleCommandDaemonize(const vector<string>& argv, const ResultCallback& result_cb) {
    put_line(argv[0], ": Not implement, use https://github.com/bmc/daemonize.git instead");
    return 0;
}

}  // namespace evt_loop
