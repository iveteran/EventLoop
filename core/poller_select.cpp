#if defined(USE_SELECT)
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/select.h>
#include "poller.h"
#include "logger.h"

namespace evt_loop {

Poller::Poller()
{
  el_logger->info("Poller: using select on linux platform");
  m_anfdmax = 0;
  m_fd_set_ri = new fd_set;
  m_fd_set_wi = new fd_set;
  FD_ZERO(m_fd_set_ri);
  FD_ZERO(m_fd_set_wi);
}

Poller::~Poller()
{
  delete m_fd_set_ri;
  delete m_fd_set_wi;
}

int Poller::Poll(uint32_t wait_ms, const PollCallback& poll_cb)
{
  int nfds = 0;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = wait_ms * 1000;

  fd_set fd_set_ro;
  fd_set fd_set_wo;

  memcpy(&fd_set_ro, m_fd_set_ri, sizeof(fd_set_ro));
  memcpy(&fd_set_wo, m_fd_set_wi, sizeof(fd_set_wo));

  int fd_setsize = m_anfdmax < FD_SETSIZE ? (m_anfdmax + 1) : FD_SETSIZE;
  int rc = select(fd_setsize, &fd_set_ro, &fd_set_wo, 0, &tv);
  if (rc > 0)
  {
    el_logger->info("[Poller:Poll] select: rc({}), m_anfdmax({})", rc, m_anfdmax);
    for (int fd = 0; fd <= m_anfdmax; ++fd)
    {
      //el_logger->debug("[Poller:Poll] fd ({})", fd);
      uint32_t events = 0;
      if (FD_ISSET(fd, &fd_set_ro)) {
        events |= FileEvent::READ;
      }
      if (FD_ISSET(fd, &fd_set_wo)) {
        events |= FileEvent::WRITE;
      }

      if (events != 0) {
        nfds++;
        auto iter = m_fd_userdata_map.find(fd);
        if (iter != m_fd_userdata_map.end()) {
          poll_cb((IOEvent*)iter->second, events, nullptr);
        } else {
          el_logger->error("the fd({}) not exists in fd userdata map", fd);
        }
      }
    }
  } else if (rc < 0) {
    el_logger->error("[Poller:Poll] select failed: rc({})", rc);
  }
  //el_logger->debug("[Poller:Poll] select: rc({}), nfds({}), wait_ms({})", rc, nfds, wait_ms);

  return nfds;
}

int Poller::SetEvents(int fd, PollerCtrl ctrl, uint32_t events, void* userdata)
{
  assert (fd < FD_SETSIZE && "Poller(select): fd >= FD_SETSIZE passed to fd_set-based select backend");
  if (fd < 0) return -1;

  el_logger->debug("[Poller::SetEvents] fd: {}, ctrl: {}, events: {}, userdata: {}", fd, ctrl, events, userdata);
  if (ctrl == PollerCtrl::DELETE) {
    if (events & FileEvent::READ)
      FD_CLR(fd, m_fd_set_ri);
    if (events & FileEvent::WRITE)
      FD_CLR(fd, m_fd_set_wi);

    auto iter = m_fd_userdata_map.find(fd);
    if (iter != m_fd_userdata_map.end()) {
      m_fd_userdata_map.erase(iter);
    }
  } else {
    //if (ctrl == PollerCtrl::ADD) {
      auto iter = m_fd_userdata_map.find(fd);
      if (iter == m_fd_userdata_map.end()) {
        m_fd_userdata_map[fd] = userdata;
      }

      if (fd > m_anfdmax) m_anfdmax = fd;
    //}

    if (events & FileEvent::READ)
      FD_SET(fd, m_fd_set_ri);
    else
      FD_CLR(fd, m_fd_set_ri);

    if (events & FileEvent::WRITE)
      FD_SET(fd, m_fd_set_wi);
    else
      FD_CLR(fd, m_fd_set_wi);
  }

  return 0;
}

}  // namespace evt_loop
#endif  // USE_SELECT
