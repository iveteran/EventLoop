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
  Console(const char* prompt = "> ", const char* output_prompt = "")
    : IOEvent(IOType::STDIN, STDIN_FILENO, FileEvent::READ | FileEvent::ERROR),
      prompt_(prompt), output_prompt_(output_prompt), os_(std::cout) {
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
  // Call Initialize before Instance if you want to set your own prompt
  static Console *Initialize(const char* prompt="", const char* output_prompt="") {
      if (!instance_) {
          instance_ = prompt ? new Console(prompt, output_prompt) : new Console();
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
      output(x);
      os_ << std::endl;
  }
  template<typename T, typename... Args>
  inline void put_line_p(const T& x, Args... args) {
      output(output_prompt_);
      output(x);
      put_line(args...);
  }
  template<typename T>
  inline void put_line_p(const T& x) {
      output(output_prompt_);
      put_line(x);
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
  int handleCommandDaemonize(const vector<string>& argv, const ResultCallback& result_cb);

  int onCommandEchoResult(int status, const string& data);

 private:
  void OnEvents(uint32_t events, void* ctx = nullptr) override;
  size_t GetLine();
  void handleCtrl_D();
  void handleInput(const string& input);
  vector<string> parseInput(const string& input);
  void registerInnerCommand();
  string verifyCommand(const string& cmd);

 private:
  string prompt_;
  string output_prompt_;
  map<string, CommandPtr> cmd_callbacks_;
  std::ostream& os_;

  static Console* instance_;
};

}  // namespace evt_loop

#endif  // _CONSOLE_H
