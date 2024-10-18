#include <stdio.h>
#include "eventloop/el.h"

using namespace evt_loop;

void PeriodicTimerHandler(TimerEvent* timer)
{
    printf("[PeriodicTimerHandler] triggered\n");
}

void OneshotTimerHandler(TimerEvent* timer)
{
    printf("[OneshotTimerHandler] triggered\n");
}

int main(int argc, char **argv)
{
#ifndef USE_SINGLETON_EVENTLOOP
    EventLoop el;
#endif

    SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
            printf("Shutdown\n");
#if defined(USE_SINGLETON_EVENTLOOP)
            printf("Use EventLoop in singleton mode!\n");
            EV_Singleton->StopLoop();
#else
            printf("Use EventLoop in stack instance mode!\n");
            el.StopLoop();
#endif
            });

    IdleEvent idle_task([](UserEvent* event, void* udata) {
            printf("[idle_task] Trigger idle event(id: %d), udata: %p, remain repeat: %d\n",
                    event->Id(), udata, event->GetRepeatRemain());
            }, NULL);
    IdleEvent idle_task2([](UserEvent* event, void* udata) {
            printf("[idle_task2] Trigger idle event(id: %d), udata: %p, remain repeat: %d\n",
                    event->Id(), udata, event->GetRepeatRemain());
            }, NULL, 5);

    TickEvent tick_task([](UserEvent* event, void* udata) {
            printf("[tick_task] Trigger tick event(id: %d), udata: %p, remain repeat: %d\n",
                    event->Id(), udata, event->GetRepeatRemain());
            }, NULL, 10);

    PeriodicTimer periodic_timer(TimeVal(5, 0), &PeriodicTimerHandler);
    periodic_timer.Start();

    OneshotTimer oneshot_timer(TimeVal(10, 0), &OneshotTimerHandler);
    oneshot_timer.Start();

#if defined(USE_SINGLETON_EVENTLOOP)
    printf("Use EventLoop in singleton mode!\n");
    EV_Singleton->StartLoop();
#else
    el.AddEvent(&idle_task);
    el.AddEvent(&idle_task2);
    el.AddEvent(&tick_task);
    el.AddEvent(&periodic_timer);
    el.AddEvent(&oneshot_timer);

    printf("Use EventLoop in stack instance mode!\n");
    el.StartLoop();
#endif

    return 0;
}
