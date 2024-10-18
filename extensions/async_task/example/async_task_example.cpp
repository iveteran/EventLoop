#include <stdio.h>
#include "eventloop/el.h"
#include "find_prime_numbers.h"
#include "../async_task.h"

using namespace evt_loop;

void PeriodicTimerHandler(TimerEvent* timer)
{
    el_logger->debug("[PeriodicTimerHandler] timer triggered");
}

int main(int argc, char **argv)
{
    SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
            printf("Shutdown\n");
            EV_Singleton->StopLoop();
            });

    //TickEvent tick_task([](UserEvent* event, void* udata) {
    //        // will be called in per tick
    //        printf("[tick_task] Trigger tick event(id: %d)\n", event->Id());
    //        }, NULL, -1);

    PeriodicTimer periodic_timer(TimeVal(5, 0), &PeriodicTimerHandler);
    periodic_timer.Start();

    PrimeParams params;
    params.range.first = 100;
    params.range.second = 1000000;

    AsyncTask async_task;
    async_task.AssignTask(find_prime_numbers, (void*)&params, &handle_prime_result);
    bool success = async_task.Run();
    if (success) {
        el_logger->info("Run async task successful, wait for the task to complete and get result");
    } else {
        el_logger->error("Run async task failed");
    }

    EV_Singleton->StartLoop();

    return 0;
}
