#ifndef _POLLER_H
#define _POLLER_H

#include <functional>

#if defined(USE_SELECT)
#include <sys/select.h>
#include <map>
#endif

namespace evt_loop {

enum FileEvent {
  READ = 1 << 0,
  WRITE = 1 << 1,
  ERROR = 1 << 2,
  CREATE = 1 << 3,
  CLOSED = 1 << 4
};

enum PollerCtrl {
  ADD,
  UPDATE,
  DELETE
};

class Poller
{
  typedef std::function<void (void*, uint32_t)> PollCallback;
  static const uint32_t MAX_EVENTS = 256;

  public:
  Poller();
  ~Poller();
  int Poll(uint32_t wait_ms, const PollCallback& poll_cb);
  int SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata = NULL);

  private:
  int pfd_;
#if defined(USE_SELECT)
  std::map<int, void*> m_fd_userdata_map;
  int m_anfdmax;
  fd_set* m_fd_set_ri;
  fd_set* m_fd_set_wi;
#endif
};

}  // ns evt_loop

#endif  // _POLLER_H
