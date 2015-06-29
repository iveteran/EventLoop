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

class ReadEvent : public BaseFileEvent {
 public:
  void OnEvents(uint32_t events) {
    if (events & BaseFileEvent::READ) {
      char a;
      if (read(file, &a, sizeof(char)) == 0) {
        close(file);
        delete this;
        return;
      }
      write(file, &a, sizeof(char));
    }

    if (events & BaseFileEvent::ERROR) {
      close(file);
      delete this;
    }
  }
};

class AcceptEvent: public BaseFileEvent {
 public:
  void OnEvents(uint32_t events) {
    if (events & BaseFileEvent::READ) {
      uint32_t size = 0;
      struct sockaddr_in addr;

      int fd = accept(file, (struct sockaddr*)&addr, &size);
      ReadEvent *e = new ReadEvent();
      e->SetFile(fd);
      e->SetEvents(BaseFileEvent::READ | BaseFileEvent::ERROR);
      el.AddEvent(e);
    }

    if (events & BaseFileEvent::ERROR) {
      close(file);
    }
  }
};

class Signal : public BaseSignalEvent {
 public:
  void OnEvents(uint32_t events) {
    printf("shutdown\n");
    el.StopLoop();
  }
};

int main(int argc, char **argv) {
  int fd;
  AcceptEvent e;

  e.SetEvents(BaseFileEvent::READ | BaseFileEvent::ERROR);

  fd = BindTo("0.0.0.0", 22222);
  if (fd == -1) {
    printf("binding address %s", strerror(errno));
    return -1;
  }

  e.SetFile(fd);

  el.AddEvent(&e);

  Signal s;
  s.SetSignal(BaseSignalEvent::INT);
  el.AddEvent(&s);

  el.StartLoop();

  return 0;
}

