#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "io_event.h"

using std::string;
using std::map;
using std::vector;

namespace evt_loop
{

#if defined(__linux__) && defined(SUPPORTS_READLINE)
void cb_line_handler(char *line);
#endif

class Console;

using CommandCallback = std::function<int (const vector<string>&)>;

struct ConsoleCommand {
    string cmd;
    string desc;
    CommandCallback callback;

    ConsoleCommand(const string& _cmd, const string& _desc, const CommandCallback& cb) :
        cmd(_cmd), desc(_desc), callback(cb)
    {}
};
using ConsoleCommandPtr = std::shared_ptr<ConsoleCommand>;

class Console : public IOEvent {
  friend void cb_line_handler(char *line);
  private:
  Console(const char* prompt = "> ")
    : IOEvent(IOType::STDIN, STDIN_FILENO, FileEvent::READ | FileEvent::ERROR),
      prompt_(prompt) {
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
  int registerCommand(const char* cmd, const char* desc, const CommandCallback& cb);

  void put_line(const string& text);

 protected:
  int handleCommand(const vector<string>& argv);

  int handleCommandHelp(const vector<string>& argv);
  int handleCommandEcho(const vector<string>& argv);
  int handleCommandChangePrompt(const vector<string>& argv);

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
  map<string, ConsoleCommandPtr> cmd_callbacks_;

  static Console* instance_;
};

}  // namespace evt_loop

#endif  // _CONSOLE_H
