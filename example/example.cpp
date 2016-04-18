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

class Periodic : public PeriodicTimerEvent {
 public:
  void OnTimer() {
    printf("Periodic timer\n");
  }
};

class Timer : public TimerEvent {
 public:
  void OnEvents(uint32_t events) {
    printf("timer:%u\n", static_cast<uint32_t>(time(0)));
    timeval tv = el.Now();
    tv.tv_sec += 1;
    SetTime(tv);
    el.UpdateEvent(this);
    if (!p) return;
    if (p->IsRunning()) p->Stop();
    else p->Start();
  }

 public:
  Periodic *p;
};

class Signal : public SignalEvent {
 public:
  void OnEvents(uint32_t events) {
    printf("shutdown\n");
    el.StopLoop();
  }
};

int main(int argc, char **argv) {
  Periodic p;
  Timer t;
  t.p = &p;

  timeval tv;
  gettimeofday(&tv, NULL);
  tv.tv_sec += 3;
  t.SetTime(tv);

  el.AddEvent(&t);

  timeval tv2;
  tv2.tv_sec = 0;
  tv2.tv_usec = 500 * 1000;
  p.SetInterval(tv2);

  el.AddEvent(&p);

  p.Start();


  Signal s;
  s.SetSignal(SignalEvent::INT);
  el.AddEvent(&s);

  el.StartLoop();

  return 0;
}

