#if defined(__linux__)
#include <unistd.h>
#include <sys/epoll.h>
#include "poller.h"

namespace evt_loop {

static uint32_t FromEpollEvents(uint32_t epoll_events);
static uint32_t ToEpollEvents(uint32_t events);
static int ToEpollCtrl(PollerCtrl ctrl);

Poller::Poller()
{
  pfd_ = epoll_create(MAX_EVENTS);
}

Poller::~Poller()
{
  close(pfd_);
  pfd_ = -1;
}

int Poller::Poll(uint32_t wait_ms, const PollCallback& poll_cb)
{
  epoll_event evs[MAX_EVENTS];
  int nfds = epoll_wait(pfd_, evs, MAX_EVENTS, wait_ms);
  for (int i = 0; i < nfds; i++) {
    uint32_t events = FromEpollEvents(evs[i].events);
    poll_cb(evs[i].data.ptr, events);
  }
  return nfds;
}

int Poller::SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata)
{
  if (fd < 0) return -1;
  epoll_event ev = {0, {0}};
  int epoll_ctrl = ToEpollCtrl(ctrl);

  if (epoll_ctrl != EPOLL_CTL_DEL) {
    ev.events = ToEpollEvents(events);
    if (userdata != NULL)
      ev.data.ptr = userdata;
  }

  return epoll_ctl(pfd_, epoll_ctrl, fd, &ev);
}

static uint32_t ToEpollEvents(uint32_t events)
{
  uint32_t epoll_events = 0;
  if (events & FileEvent::READ) epoll_events |= EPOLLIN;
  if (events & FileEvent::WRITE) epoll_events |= EPOLLOUT;
  if (events & FileEvent::CLOSED) epoll_events |= EPOLLRDHUP;
  if (events & FileEvent::ERROR) epoll_events |= EPOLLHUP | EPOLLERR;
  return epoll_events;
}

static uint32_t FromEpollEvents(uint32_t epoll_events)
{
  uint32_t events = 0;
  if (epoll_events & EPOLLIN) events |= FileEvent::READ;
  if (epoll_events & EPOLLOUT) events |= FileEvent::WRITE;
  if (epoll_events & EPOLLRDHUP) events |= FileEvent::CLOSED;
  if (epoll_events & (EPOLLHUP | EPOLLERR)) events |= FileEvent::ERROR;
  return events;
}

static int ToEpollCtrl(PollerCtrl ctrl)
{
  return ctrl == PollerCtrl::ADD ? EPOLL_CTL_ADD :
      ctrl == PollerCtrl::UPDATE ? EPOLL_CTL_MOD :
      ctrl == PollerCtrl::DELETE ? EPOLL_CTL_DEL :
      0;
}

}  // namespace evt_loop
#endif  // __linux__
