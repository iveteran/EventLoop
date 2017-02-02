#ifndef _SIGNAL_HANDLER_H
#define _SIGNAL_HANDLER_H

#include <signal.h>
#include <set>
#include <map>
#include <functional>
#include "event.h"

using std::set;
using std::map;
using std::function;

namespace evt_loop
{

class SignalEvent : public IEvent {
  friend class EventLoop;
 public:
  enum SIGNO {
      UNDEFINED = 0,
      HUP       = SIGHUP,
      INT       = SIGINT,
      QUIT      = SIGQUIT,
      ILL       = SIGILL,
      TRAP      = SIGTRAP,
      ABRT      = SIGABRT,
      BUS       = SIGBUS,
      FPE       = SIGFPE,
      KILL      = SIGKILL,
      USR1      = SIGUSR1,
      SEGV      = SIGSEGV,
      USR2      = SIGUSR2,
      PIPE      = SIGPIPE,
      ALRM      = SIGALRM,
      TERM      = SIGTERM,
      //STKFLT    = SIGSTKFLT,
      CHLD      = SIGCHLD,
      CONT      = SIGCONT,
      STOP      = SIGSTOP,
      TSTP      = SIGTSTP,
      TTIN      = SIGTTIN,
      TTOU      = SIGTTOU,
      URG       = SIGURG,
      XCPU      = SIGXCPU,
      XFSZ      = SIGXFSZ,
      VTALRM    = SIGVTALRM,
      PROF      = SIGPROF,
      WINCH     = SIGWINCH,
      IO        = SIGIO,
      //PWR       = SIGPWR,
      SYS       = SIGSYS,
  };

 public:
  SignalEvent(SIGNO signo) : sig_no_(signo) {}

 public:
  void SetSignal(SIGNO sig_no) { sig_no_ = sig_no; }
  SIGNO Signal() const { return sig_no_; }

 protected:
  SIGNO sig_no_;
};

class SignalHandler : public SignalEvent
{
  typedef function<void (SignalHandler*, uint32_t)>   OnSignalCallback;
  public:
  SignalHandler(SIGNO signo, const OnSignalCallback& cb);
  ~SignalHandler();

  private:
  void OnEvents(uint32_t events) {
    printf("SignalHandler receives signal (%d).\n", events);
    signal_cb_(this, events);
  }

  private:
  OnSignalCallback   signal_cb_;
};

// singleton class that manages all signals
class SignalManager {
 public:
  int AddEvent(SignalEvent *e);
  int DeleteEvent(SignalEvent *e);
  int UpdateEvent(SignalEvent *e);

 private:
  friend void HandleSignal(int signo);
  map<int, set<SignalEvent *> > sig_events_;

 public:
  static SignalManager *Instance() {
    if (!instance_) {
      instance_ = new SignalManager();
    }
    return instance_;
  }
 private:
  SignalManager() {}
 private:
  static SignalManager* instance_;
};

}  // namespace evt_loop

#endif  // _SIGNAL_HANDLER_H
