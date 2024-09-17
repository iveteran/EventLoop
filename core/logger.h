#ifndef _LOGGER_H
#define _LOGGER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <memory>
#include <functional>
#include <cassert>
#include <cerrno>
#include <cstring>

#define EOS '\0'

using namespace std;

namespace evt_loop {

// Enum to represent log levels
enum class LogLevel {
    NONE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
};

static const char* ColorOff        = "\033[0m";     // Color Reset
static const char* ColorRed        = "\033[31m";    // Red
static const char* ColorGreen      = "\033[32m";    // Green
static const char* ColorYellow     = "\033[33m";    // Yellow
static const char* ColorBlue       = "\033[34m";    // Blue
static const char* ColorBgRed      = "\033[41m";    // Red Background

using ostream_ptr = std::unique_ptr<std::ostream, std::function<void(std::ostream*)>>;

class Logger {
public:
    Logger(std::ostream& os = std::cout, bool colorful = true) :
        is_logfile_(false),
        os_(&os, [](ostream*){}),
        colorful_(colorful)
    {
        init();
    }

    Logger(const std::string& filename, bool colorful = false) :
        filename_(filename),
        is_logfile_(true),
        os_(new std::ofstream(filename, std::ios::app), std::default_delete<std::ostream>()),
        colorful_(colorful)
    {
        //if (!dynamic_cast<std::ofstream&>(*os_).is_open()) {
        //    // do errorry things and don't have to deallocate os
        //}
        assert (dynamic_cast<std::ofstream&>(*os_).is_open());
        _add_viewing_logfile_tips();
        init();
    }

    ~Logger() { }

    void init()
    {
    }

    void close()
    {
        if (! os_) {
            return;
        }
        if (is_logfile_) {
            dynamic_cast<std::ofstream&>(*os_).close();
            is_logfile_ = false;
        }
        os_ = nullptr;
    }

    void flush()
    {
        *os_ << std::flush;
    }

    string get_info() const {
        stringstream ss;
        ss << "On/Off: " << (disabled_ ? "Off" : "On") << "\n";
        ss << "Colorful: " << (colorful_ ? "On" : "Off") << "\n";
        ss << "Output: " << (is_logfile_ ? "logfile" : "STDOUT") << "\n";
        ss << "Logfile: " << filename_;
        return ss.str();
    }

    void enable_colorful(bool value = true)
    {
        colorful_ = value;
    }
    bool is_colorful() const {
        return colorful_;
    }

    void disable(bool value = true) {
        disabled_ = value;
    }
    bool is_disabled() const {
        return disabled_;
    }

    void switch_to_stdout()
    {
        if (is_logfile_) {
            dynamic_cast<std::ofstream&>(*os_).close();
            is_logfile_ = false;
        }
        os_ = ostream_ptr(&std::cout, [](ostream*){});
    }

    void switch_to_file(const string& filename)
    {
        if (is_logfile_ && filename == filename_) {
            cout << "error: switch to file " << filename << " failure, the file already for current log." << endl;
            return;
        }
        if (os_) {
            flush();
        }
        auto ofs = new std::ofstream(filename, std::ios::app);
        if (ofs->fail()) {
            // show error message an do nothing
            cout << "error: open file " << filename << " failure, " << strerror(errno) << endl;
        } else {
            if (os_ && is_logfile_) {
                // close previous log file
                dynamic_cast<std::ofstream&>(*os_).close();
            }
            os_ = ostream_ptr(ofs, std::default_delete<std::ostream>());

            filename_ = filename;
            is_logfile_ = true;
            _add_viewing_logfile_tips();
        }
    }

    template<typename T, typename... Args>
    Logger& info(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        return log(LogLevel::INFO, fmt_str, x, args...);
    }
    template<typename T, typename... Args>
    Logger& debug(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        return log(LogLevel::DEBUG, fmt_str, x, args...);
    }
    template<typename T, typename... Args>
    Logger& warn(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        return log(LogLevel::WARN, fmt_str, x, args...);
    }
    template<typename T, typename... Args>
    Logger& error(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        return log(LogLevel::ERROR, fmt_str, x, args...);
    }
    template<typename T, typename... Args>
    Logger& critical(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        return log(LogLevel::CRITICAL, fmt_str, x, args...);
    }
    template<typename T, typename... Args>
    Logger& output(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        _mprintf(fmt_str, x, args...);
        return *this;
    }

    template<typename T>
    Logger& info(std::string_view fmt_str, const T& x) throw()
    {
        return log(LogLevel::INFO, fmt_str, x);
    }
    template<typename T>
    Logger& debug(std::string_view fmt_str, const T& x) throw()
    {
        return log(LogLevel::DEBUG, fmt_str, x);
    }
    template<typename T>
    Logger& warn(std::string_view fmt_str, const T& x) throw()
    {
        return log(LogLevel::WARN, fmt_str, x);
    }
    template<typename T>
    Logger& error(std::string_view fmt_str, const T& x) throw()
    {
        return log(LogLevel::ERROR, fmt_str, x);
    }
    template<typename T>
    Logger& critical(std::string_view fmt_str, const T& x) throw()
    {
        return log(LogLevel::CRITICAL, fmt_str, x);
    }
    template<typename T>
    Logger& output(std::string_view fmt_str, const T& x) throw()
    {
        _mprintf(fmt_str, x);
        return *this;
    }

    inline Logger& info(std::string_view fmt_str) throw()
    {
        return log(LogLevel::INFO, fmt_str);
    }
    inline Logger& debug(std::string_view fmt_str) throw()
    {
        return log(LogLevel::DEBUG, fmt_str);
    }
    inline Logger& warn(std::string_view fmt_str) throw()
    {
        return log(LogLevel::WARN, fmt_str);
    }
    inline Logger& error(std::string_view fmt_str) throw()
    {
        return log(LogLevel::ERROR, fmt_str);
    }
    inline Logger& critical(std::string_view fmt_str) throw()
    {
        return log(LogLevel::CRITICAL, fmt_str);
    }
    inline Logger& output(std::string_view fmt_str) throw()
    {
        _mprintf(fmt_str);
        return *this;
    }

    inline Logger& eol() throw()
    {
        *os_ << endl;
        return *this;
    }

    template<typename T, typename... Args>
    Logger& log(LogLevel level, std::string_view fmt_str, const T& x, Args... args) throw()
    {
        _print_prefix(level);
        _mprintf(fmt_str, x, args...);
        *os_ << endl;
        return *this;
    }

    template<typename T>
    Logger& log(LogLevel level, std::string_view fmt_str, const T& x) throw()
    {
        _print_prefix(level);
        _mprintf(fmt_str, x);
        *os_ << endl;
        return *this;
    }

    inline Logger& log(LogLevel level, std::string_view fmt_str) throw()
    {
        _print_prefix(level);
        _mprintf(fmt_str);
        *os_ << endl;
        return *this;
    }

private:
    template<typename T, typename... Args>
    void _mprintf(std::string_view fmt_str, const T& x, Args... args) throw()
    {
        if (disabled_ || ! os_) {
            return;
        }
        size_t pos = _mprintf_item(fmt_str, x);
        _mprintf(&fmt_str[pos+2], args...);
    }

    template<typename T>
    void _mprintf(std::string_view fmt_str, const T& x) throw()
    {
        if (disabled_ || ! os_) {
            return;
        }
        size_t pos = _mprintf_item(fmt_str, x);
        // print the rest of the stirng
        *os_ << &fmt_str[pos+2];
    }

    inline void _mprintf(std::string_view str) throw()
    {
        if (disabled_ || ! os_) {
            return;
        }
        *os_ << str;
    }

    template<typename T>
    size_t _mprintf_item(std::string_view fmt_str, T& x)
    {
        size_t pos = 0;
        for (; fmt_str[pos] != EOS; pos++) {
            char c = fmt_str[pos];
            char next_c = fmt_str[pos+1];
            if (c == '{' && next_c == '}') {
                break;
            }
            if (c == '\\') {
                continue;
            }

            *os_ << c;

            if (next_c == EOS) {
                *os_ << next_c;
                return fmt_str.size();
            }
        }
        *os_ << x;
        return pos;
    }

    void _print_prefix(LogLevel level)
    {
        // Get current timestamp
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        *os_ << "[" << timestamp << "] "
                << levelToString(level) << ": ";
    }

    string levelToString(LogLevel level)
    {
        const char* levelText = NULL;
        const char* color = NULL;
        switch (level) {
        case LogLevel::DEBUG:
            levelText = "DEBUG";
            color = ColorBlue;
            break;
        case LogLevel::INFO:
            levelText = "INFO";
            color = ColorGreen;
            break;
        case LogLevel::WARN:
            levelText = "WARN";
            color = ColorYellow;
            break;
        case LogLevel::ERROR:
            levelText = "ERROR";
            color = ColorRed;
            break;
        case LogLevel::CRITICAL:
            levelText = "CRITICAL";
            color = ColorBgRed;
            break;
        default:
            levelText = "UNKNOWN";
            break;
        }

        if (colorful_) {
            return colourText(levelText, color);
        } else {
            return levelText;
        }
    }

    string colourText(const char* text, const char* color)
    {
        if (! color) {
            return text;
        }
#if defined(__linux__)
        stringstream ss;
        ss << color << text << ColorOff;
        return ss.str();
#else
        return text;
#endif
    }

    void _add_viewing_logfile_tips()
    {
        *os_ << "TIPS(for Linux): use bat(a text viewer) to view log with colorful level" << endl;
    }

private:
    string filename_;
    bool is_logfile_;
    ostream_ptr os_;
    bool colorful_;
    bool disabled_;
};

}  // namespace evt_loop

#include "../core/singleton_tmpl.h"
#define el_logger   (Singleton<Logger>::GetInstance())  // Default logger

#endif  // _LOGGER_H
