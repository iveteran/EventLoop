#include <errno.h>
#include <iostream>
#include "console.h"

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

size_t get_max_size_of_words(const map<string, ConsoleCommandPtr>& cmds) {
    size_t max_size = 0;
    for (auto [cmd_name, _] : cmds) {
        if (cmd_name.size() > max_size) {
            max_size = cmd_name.size();
        }
    }
    return max_size;
}

void Console::OnEvents(uint32_t events) {
    if (events & FileEvent::READ) {
        GetLine();
    }

    if (events & FileEvent::CLOSED) {
        OnClosed();
    } else if ((events & FileEvent::ERROR)) {
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
    cout << prompt_ << flush;
    return input.size();
}

void Console::handleCtrl_D() {
    cout << "Pressed Ctrl-D" << endl;
    cin.clear();
    clearerr(stdin);   // 重置输入流的状态
}

void Console::put_line(const string& text) {
  //cout << prompt_ << text << endl;
  cout << text << endl;
}

int Console::registerCommand(const char* cmd, const char* desc, const CommandCallback& cb) {
    string errmsg = verifyCommand(cmd);
    if (! errmsg.empty()) {
        fprintf(stderr, "[Console::registerCommand] Illegal command name: %s, reason: %s", cmd, errmsg.c_str());
        return -1;
    }
    string _cmd = to_lower(cmd);
    auto cmd_obj = std::make_shared<ConsoleCommand>(_cmd, desc, cb);
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
    return cmd_obj->callback(argv);
}

void Console::registerInnerCommand() {
    registerCommand(
            ".help",
            "print this help",
            std::bind(&Console::handleCommandHelp, this, std::placeholders::_1)
            );
    registerCommand(
            ".echo",
            "echo input, .echo [text]",
            std::bind(&Console::handleCommandEcho, this, std::placeholders::_1)
            );
    registerCommand(
            ".change_prompt",
            "change console prompt, example: .change_prompt \"$ \"",
            std::bind(&Console::handleCommandChangePrompt, this, std::placeholders::_1)
            );
}

int Console::handleCommandHelp(const vector<string>& argv) {
    size_t max_size_of_cmd_name = get_max_size_of_words(cmd_callbacks_);
    auto twice_spaces = std::string(2, ' ');
    cout << "Commands: " << endl;
    for (auto [cmd_name, cmd_obj] : cmd_callbacks_) {
        size_t space_times = max_size_of_cmd_name - cmd_name.size() + 2;
        auto some_spaces = std::string(space_times, ' ');
        cout << twice_spaces << cmd_name << some_spaces << cmd_obj->desc << endl;
    }
    return 0;
}

int Console::handleCommandEcho(const vector<string>& argv) {
    for (size_t i=1; i<argv.size(); i++) {
        cout << argv[i];
        if (i < argv.size() - 1) {
            cout << ' ';
        }
    }
    cout << endl;
    return 0;
}

int Console::handleCommandChangePrompt(const vector<string>& argv) {
    if (argv.size() > 1) {
        auto new_prompt = argv[1];
        if (prompt_ != new_prompt) {
            prompt_ = new_prompt;
        }
    } else {
        cout << "Invalid parameter" << endl;
    }
    return 0;
}

}  // namespace evt_loop
