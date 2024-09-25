#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>

#include "eventloop/io_event.h"

using std::string;
using std::map;
using std::vector;

namespace evt_loop
{

#if defined(__linux__) && defined(SUPPORTS_READLINE)
void cb_line_handler(char *line);
#endif

class Console;

class Console : public IOEvent {
  friend void cb_line_handler(char *line);

public:
  using ResultCallback = std::function<int (int, const string&)>;
  using CommandCallback = std::function<int (const vector<string>&, const ResultCallback&)>;

  struct Command {
    string cmd;
    string desc;
    CommandCallback callback;
    ResultCallback result_callback;

    Command(const string& _cmd, const string& _desc,
            const CommandCallback& cb, const ResultCallback& result_cb=nullptr)
        : cmd(_cmd), desc(_desc), callback(cb), result_callback(result_cb)
    {}
  };
  using CommandPtr = std::shared_ptr<Command>;

  private:
  Console(const char* prompt = "> ")
    : IOEvent(IOType::STDIN, STDIN_FILENO, FileEvent::READ | FileEvent::ERROR),
      prompt_(prompt), os_(std::cout) {
    init();
    registerInnerCommand();
  }

  public:
  static Console *Instance() {
    if (!instance_) {
      instance_ = new Console();
    }
    return instance_;
  }

  virtual ~Console() { destory(); }

  void init();
  void destory();
  int registerCommand(const char* cmd, const char* desc,
          const CommandCallback& cb, const ResultCallback& result_cb=nullptr);

  template<typename T, typename... Args>
  inline void put_line(const T& x, Args... args) {
      output(x);
      output(args...);
      os_ << std::endl;
  }
  template<typename T>
  inline void put_line(const T& x) {
      os_ << x << std::endl;
  }
  template<typename T, typename... Args>
  inline void output(const T& x, Args... args) {
      output(x);
      output(args...);
  }
  template<typename T>
  inline void output(const T& x) {
      os_ << x;
  }

 protected:
  int handleCommand(const vector<string>& argv);

  int handleCommandHelp(const vector<string>& argv, const ResultCallback& result_cb);
  int handleCommandEcho(const vector<string>& argv, const ResultCallback& result_cb);
  int handleCommandChangePrompt(const vector<string>& argv, const ResultCallback& result_cb);
  int handleCommandLog(const vector<string>& argv, const ResultCallback& result_cb);

  int onCommandEchoResult(int status, const string& data);

 private:
  void OnEvents(uint32_t events);
  size_t GetLine();
  void handleCtrl_D();
  void handleInput(const string& input);
  vector<string> parseInput(const string& input);
  void registerInnerCommand();
  string verifyCommand(const string& cmd);

 private:
  string prompt_;
  map<string, CommandPtr> cmd_callbacks_;
  std::ostream& os_;

  static Console* instance_;
};

}  // namespace evt_loop

#endif  // _CONSOLE_H
