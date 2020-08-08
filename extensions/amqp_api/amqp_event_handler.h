#ifndef _AMQP_EVENT_HANDLER_H
#define _AMQP_EVENT_HANDLER_H 

#include "el.h"
#include "amqpcpp.h"
#include "amqpcpp/linux_tcp.h"

using namespace evt_loop;

namespace amqp_api {

class AMQPEventHandler : public AMQP::TcpHandler
{
  class Watcher : public IOEvent
  {
    public:
      Watcher(AMQP::TcpConnection* connection, int fd, int amqp_events) : amqp_connection_(connection)
      {
        uint32_t io_events = amqp_to_io_events(amqp_events);
        WatchEvents(fd, io_events);
      }

     Watcher(Watcher &&that) = delete;
     Watcher(const Watcher &that) = delete;

     void UpdateEvents(int events)
     {
        uint32_t io_events = amqp_to_io_events(events);
        IOEvent::UpdateEvents(io_events);
     }

    protected:
      virtual void OnEvents(uint32_t events) override
      {
        int amqp_events = io_to_amqp_events(events);
        amqp_connection_->process(fd_, amqp_events);
      }

    private:
      static int io_to_amqp_events(uint32_t events)
      {
        int amqp_events = 0;
        if (events & FileEvent::READ)
          amqp_events |= AMQP::readable;
        if (events & FileEvent::WRITE)
          amqp_events |= AMQP::writable;
        return amqp_events;
      }

      static uint32_t amqp_to_io_events(int events)
      {
        int io_events = 0;
        if (events & AMQP::readable)
          io_events |= FileEvent::READ;
        if (events & AMQP::writable)
          io_events |= FileEvent::WRITE;
        return io_events;
      }

    private:
      AMQP::TcpConnection* amqp_connection_;
  };

  public:
    AMQPEventHandler() { }
    virtual ~AMQPEventHandler() = default;

  private:
    virtual void monitor(AMQP::TcpConnection *connection, int fd, int flags) override
    {
      auto iter = watchers_.find(fd);

      if (iter == watchers_.end()) {
        if (flags == 0) return;
        watchers_[fd] = std::unique_ptr<Watcher>(new Watcher(connection, fd, flags));
      } else if (flags == 0) {
        watchers_.erase(iter);
      } else {
        iter->second->UpdateEvents(flags);
      }
    }

  private:
    std::map<int,std::unique_ptr<Watcher>> watchers_;

};

}

#endif  // _AMQP_EVENT_HANDLER_H
