#if defined(__macosx__) || defined(__darwin__) || defined(__freebsd__)
#include <unistd.h>
#include "poller.h"

#define EVENT_NUM;

namespace evt_loop {

static void AddKqueueEvents(struct kevent& ev[], int n_ev, void* userdata);
static void DeleteKqueueEvents(struct kevent& ev[], int n_ev);
static void UpdateKqueueEvents(struct kevent& ev[], int n_ev, void* userdata);
static uint32_t FromKqueueEvents(int kqueue_filter, int kqueue_flags);

Poller::Poller()
{
  pfd_ = kqueue();
}

Poller::~Poller()
{
  close(pfd_);
  pfd_ = -1;
}

int Poller::Poll(uint32_t wait_ms, const PollCallback& poll_cb)
{
  struct timespec timeout;
  timeout.tv_sec = wait_ms / 1000;
  timeout.tv_nsec = (wait_ms % 1000) * 1000 * 1000;
  struct kevent evs[MAX_EVENTS];
  int nfds = kevent(pfd_, NULL, 0, evs, MAX_EVENTS, &timeout);
  for (int i = 0; i < nfds; i++) {
    uint32_t events = FromKqueueEvents(evs[i].filter, evs[i].flags);
    poll_cb(evs[i].udata, events);
  }
  return nfds;
}

int Poller::SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata)
{
  if (fd < 0) return -1;
  struct kevent ev[EVENT_NUM] = {0};

  if (ctrl == PollerCtrl::ADD) {
    AddKqueueEvents(ev, EVENT_NUM, userdata);
  } else if (ctrl == PollerCtrl::DELETE) {
    DeleteKqueueEvents(ev, EVENT_NUM);
  } else if (ctrl == PollerCtrl::UPDATE) {
    UpdateKqueueEvents(ev, EVENT_NUM, userdata);
  }

  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = 0;
  return kevent(pfd_, ev, EVENT_NUM, NULL, 0, &timeout);
}

static void AddKqueueEvents(struct kevent& ev[], int n_ev, void* userdata)
{
  int n = 0;
  if (events & FileEvent::READ)
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, userdata);
  if (events & FileEvent::WRITE)
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, userdata);
}

static void DeleteKqueueEvents(struct kevent& ev[], int n_ev)
{
  int n = 0;
  if (events & FileEvent::READ)
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  if (events & FileEvent::WRITE)
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
}

static void UpdateKqueueEvents(struct kevent& ev[], int n_ev, void* userdata)
{
  int n = 0;
  if (events & FileEvent::READ)
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, userdata);
  else
    EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);

  if (events & FileEvent::WRITE)
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, userdata);
  else
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
}

static uint32_t FromKqueueEvents(int kqueue_filter, int kqueue_flags)
{
  uint32_t events = 0;
  if (kqueue_filter == EVFILT_READ) events |= FileEvent::READ;
  else if (kqueue_filter == EVFILT_WRITE && !(kqueue_flags & EV_EOF)) events |= FileEvent::WRITE;

  if (kqueue_flags & EV_EOF) events |= FileEvent::CLOSED;
  if (kqueue_flags & EV_ERROR) events |= FileEvent::ERROR;

  return events;
}

}  // namespace evt_loop
#endif
