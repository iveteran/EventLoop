#ifndef EVENT_LOOP_H_
#define EVENT_LOOP_H_

#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <sys/epoll.h>
#include <string>
#include <list>

using std::string;

namespace richinfo {

class EventLoop;
class SignalManager;

class IEvent {
  friend class EventLoop;
  friend class SignalManager;
 public:
  static const uint32_t  NONE = 0;
  static const uint32_t  ONESHOT = 1 << 30;
  static const uint32_t  TIMEOUT = 1 << 31;

 public:
  IEvent(uint32_t events = 0) : el_(NULL) { events_ = events; }

  virtual ~IEvent() {};

  virtual void OnEvents(uint32_t events) = 0;

 public:
  virtual void SetEvents(uint32_t events) { events_ = events; }
  virtual uint32_t Events() const { return events_; }

 protected:
  uint32_t events_;
  EventLoop *el_;
};

class IOEvent : public IEvent {
  friend class EventLoop;
 public:
  static const uint32_t  READ = 1 << 0;
  static const uint32_t  WRITE = 1 << 1;
  static const uint32_t  ERROR = 1 << 2;
  static const uint32_t  CREATE = 1 << 3;
  static const uint32_t  CLOSED = 1 << 4;

 public:
  IOEvent(uint32_t events = IEvent::NONE) : IEvent(events), fd_(-1) {}
  virtual ~IOEvent() { };

 public:
  void SetFD(int fd) { fd_ = fd; }
  int FD() const { return fd_; }

 protected:
  virtual void OnCreated(int fd) {};
  virtual void OnClosed() {};
  virtual void OnError(const char* errstr) {};

  virtual void OnEvents(uint32_t events) = 0;

 protected:
  int fd_;
};

class BufferIOEvent : public IOEvent {
  friend class EventLoop;
 public: BufferIOEvent(uint32_t events = IOEvent::READ | IOEvent::ERROR)
    :IOEvent(events), torecv_(0), sent_(0)/*, el_(NULL)*/ {
  }

 public:
  void SetReceiveLen(uint32_t len);
  void Send(const char *buffer, uint32_t len);
  void Send(const string& buffer);

 protected:
  virtual void OnReceived(const string& recvbuf) {};
  virtual void OnSent(const string& sentbuf) {};

 private:
  void OnEvents(uint32_t events);
  int ReceiveData(string& rtn_data);
  int SendData();

 private:
  string recvbuf_;
  uint32_t torecv_;

  std::list<string> sendbuf_list_;
  uint32_t sent_;
};

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
      STKFLT    = SIGSTKFLT,
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
      PWR       = SIGPWR,
      SYS       = SIGSYS,
  };

 public:
  SignalEvent(uint32_t events = IEvent::NONE) : IEvent(events) {}

 public:
  void SetSignal(SIGNO sig_no) { sig_no_ = sig_no; }
  SIGNO Signal() const { return sig_no_; }

 protected:
  SIGNO sig_no_;
};

class TimerEvent : public IEvent {
  friend class EventLoop;
 public:
  static const uint32_t TIMER = 1 << 0;

 public:
  TimerEvent(uint32_t events = IEvent::NONE) : IEvent(events) {}

 public:
  void SetTime(timeval tv) { time_ = tv; }
  timeval Time() const { return time_; }

 protected:
  timeval time_;
};

class PeriodicTimerEvent : public TimerEvent {
  friend class EventLoop;
 public:
  PeriodicTimerEvent() :TimerEvent(IEvent::NONE)/*, el_(NULL)*/ {};
  PeriodicTimerEvent(timeval inter) :TimerEvent(IEvent::NONE), interval_(inter)/*, el_(NULL)*/ {};

  void SetInterval(timeval inter) { interval_ = inter; }
  timeval GetInterval() const { return interval_; }

  void Start();
  void Stop();

  bool IsRunning() { return running_; }

  virtual void OnTimer() = 0;

 private:
  void OnEvents(uint32_t events);

 private:
  timeval interval_;
  bool running_;
};

class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

 public:
  // add delete & update event objects
  int AddEvent(IOEvent *e);
  int DeleteEvent(IOEvent *e);
  int UpdateEvent(IOEvent *e);

  int AddEvent(TimerEvent *e);
  int DeleteEvent(TimerEvent *e);
  int UpdateEvent(TimerEvent *e);

  int AddEvent(SignalEvent *e);
  int DeleteEvent(SignalEvent *e);
  int UpdateEvent(SignalEvent *e);

  int AddEvent(BufferIOEvent *e);
  int AddEvent(PeriodicTimerEvent *e);

  // do epoll_waite and collect events
  int ProcessEvents(int timeout);

  // event loop control
  void StartLoop();
  void StopLoop();

  timeval Now() const { return now_; }

 private:
  int CollectFileEvents(int timeout);
  int DoTimeout();

 private:
  int epfd_;
  epoll_event evs_[256];

  timeval now_;
  bool stop_;

  void *timermanager_;
};

int SetNonblocking(int fd);

}  // ns richinfo

#include "singleton_tmpl.h"
#define EV_Singleton       (Singleton<EventLoop>::GetInstance())

#endif // EVENT_LOOP_H_
