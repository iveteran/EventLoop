#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "eventloop.h"

using namespace eventloop;

EventLoop el;

class EchoConnection : public BufferIOEvent {
 public:
    void OnReceived(const std::string& buffer) {
        printf("[EchoConnection::OnReceived] received string: %s\n", buffer.c_str());
        Send(buffer);
    }

    void OnClosed() {
      printf("[EchoConnection::OnClosed] client leave, fd: %d\n", file);
      el.DeleteEvent(this);
      delete this;
    }

    void OnError(char* errstr) {
      printf("[EchoConnection::OnError] error string: %s\n", errstr);
      //close(file);
    }
};

class EchoServer: public IOEvent {
 public:
  void OnEvents(uint32_t events) {
    if (events & IOEvent::READ) {
      uint32_t size = 0;
      struct sockaddr_in addr;

      int fd = accept(file, (struct sockaddr*)&addr, &size);
      EchoConnection *e = new EchoConnection();
      printf("[EchoServer::OnEvents] new client, fd: %d\n", file);
      e->SetFile(fd);
      e->SetEvents(IOEvent::READ | IOEvent::ERROR);
      el.AddEvent(e);
    }

    if (events & IOEvent::ERROR) {
      close(file);
    }
  }
};

class Signal : public SignalEvent {
 public:
  void OnEvents(uint32_t events) {
    printf("shutdown\n");
    el.StopLoop();
  }
};

int main(int argc, char **argv) {
  int fd;
  EchoServer e;

  e.SetEvents(IOEvent::READ | IOEvent::ERROR);

  fd = BindTo("0.0.0.0", 22222);
  if (fd == -1) {
    printf("binding address %s", strerror(errno));
    return -1;
  }

  e.SetFile(fd);

  el.AddEvent(&e);

  Signal s;
  s.SetSignal(SignalEvent::INT);
  el.AddEvent(&s);

  el.StartLoop();

  return 0;
}

