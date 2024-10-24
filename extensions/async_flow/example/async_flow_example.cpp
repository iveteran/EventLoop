#include "async_executor_examples.h"
#include "flow_scheduler.h"
#include "eventloop/el.h"

using namespace evt_loop;

static int timer_times = 0;

void PeriodicTimerHandler(TimerEvent* timer)
{
    timer_times++;
    /*
    if (timer_count % 5 == 0) {
        printf("[PeriodicTimerHandler] triggered\n");
    } else {
        printf(".\n");
    }
    */
    printf("[PeriodicTimerHandler] triggered, times: %d\n", timer_times);
}

int main()
{
    SignalHandler sh(SignalEvent::INT, [&](SignalHandler* sh, uint32_t signo) {
            printf("Shutdown\n");
            printf("The EventLoop in stack instance mode!\n");
            EV_Singleton->StopLoop();
            });

#ifdef TEST_ASYNC_FLOW_WITH_SCHEDULER
    FlowScheduler scheduler;

    // flow 1
    AsyncFlow af("demo_flow");
    scheduler.AddFlow(&af);

    FlowNode root("root", &af);
    af.SetFlowRoot(&root);

    auto executor_s1 = AsyncExecutor();
    FlowNode node_s1("node_s1", &af, &executor_s1);
    root.AddNode(&node_s1);

    auto executor_s2 = DemoAsyncExecutor();
    FlowNode node_s2("node_s2", &af, &executor_s2);
    root.AddNode(&node_s2);

        auto executor_s2_s1 = AsyncExecutor();
        FlowNode node_s2_s1("node_s2_s1", &af, &executor_s2_s1);
        node_s2.AddNode(&node_s2_s1);

        auto executor_s2_s2 = DemoAsyncExecutor(3);
        FlowNode node_s2_s2("node_s2_s2", &af, &executor_s2_s2);
        node_s2.AddNode(&node_s2_s2);

            auto executor_s2_s2_s1 = DemoAsyncExecutor(10);
            FlowNode node_s2_s2_s1("node_s2_s2_s1", &af, &executor_s2_s2_s1);
            node_s2_s2.AddNode(&node_s2_s2_s1);

    auto executor_s3 = DemoAsyncExecutor(7);
    FlowNode node_s3("node_s3", &af, &executor_s3);
    root.AddNode(&node_s3);

#ifdef TEST_ASYNC_FLOW_WITH_MULTIPLE_FLOWS
    // flow 2
    AsyncFlow af_2("demo_flow_2");
    scheduler.AddFlow(&af_2);

    FlowNode f2_root("f2_root", &af_2);
    af_2.SetFlowRoot(&f2_root);

    auto f2_executor_s1 = AsyncExecutor();
    FlowNode f2_node_s1("f2_node_s1", &af_2, &f2_executor_s1);
    f2_root.AddNode(&f2_node_s1);

        auto f2_executor_s1_s1 = DemoAsyncExecutor(7);
        FlowNode f2_node_s1_s1("f2_node_s1_s1", &af_2, &f2_executor_s1_s1);
        f2_node_s1.AddNode(&f2_node_s1_s1);

            auto f2_executor_s1_s1_s1 = DemoAsyncExecutor();
            FlowNode f2_node_s1_s1_s1("f2_node_s1_s1_s1", &af_2, &f2_executor_s1_s1_s1);
            f2_node_s1_s1.AddNode(&f2_node_s1_s1_s1);
#endif

    scheduler.Run();
#endif

#ifdef TEST_ASYNC_FLOW_WITHOUT_SCHEDULER
    // flow 3, standalone mode, not be managed by FlowScheduler
    AsyncFlow af_3("demo_flow_3");
    FlowNode f3_root("f3_root", &af_3);
    af_3.SetFlowRoot(&f3_root);
    af_3.SetFinishedCallback(
            [](AsyncFlow* flow) {
            auto results = flow->GetResults();
            //if (! results) return;
            auto results_count = results->size();
            std::cout << "> flow result count: " << results_count << std::endl;

            auto v1 = std::any_cast<int>((*results)[0]);
            std::cout << "> flow result 1: " << v1 << std::endl;

            auto v2 = std::any_cast<float>((*results)[1]);
            std::cout << "> flow result 2: " << v2 << std::endl;

            auto v3 = std::any_cast<const char*>((*results)[2]);
            std::cout << "> flow result 3: " << v3 << std::endl;
            });

#if 0
    DatabaseAsyncQuerier db_querier;
    auto f3_executor_s1 = TemplatedAsyncExecutor();
    auto cb = std::bind(&DatabaseAsyncQuerier::Query, &db_querier, _1, _2);
    f3_executor_s1.SetCallee(cb);
    f3_executor_s1.AddParam("select * from tbl_dummary");
    f3_executor_s1.AddParam(std::vector<any>{1, "hello"});
    //db_querier.SetResultCallback(std::bind(&TemplatedAsyncExecutor::OnResult, &f3_executor_s1, _1, _2, _3));
#else
    //auto f3_executor_s1 = AsyncExecutor();
    auto f3_executor_s1 = DatabaseAsyncExecutor();
#endif
    f3_executor_s1.AddFinishedCallback(
            [](AsyncExecutor* executor) {
                auto results_count = executor->GetResultsCount();
                std::cout << "> f3_s1 executor result count: " << results_count << std::endl;

                auto v1 = executor->template GetResult<int>(0);
                std::cout << "> f3_s1 result 1: " << v1 << std::endl;

                auto v2 = executor->template GetResult<float>(1);
                std::cout << "> f3_s1 result 2: " << v2 << std::endl;

                auto v3 = executor->template GetResult<const char*>(2);
                std::cout << "> f3_s1 result 3: " << v3 << std::endl;
            });

    FlowNode f3_node_s1("f3_node_s1", &af_3, &f3_executor_s1);
    f3_root.AddNode(&f3_node_s1);

        auto f3_executor_s1_s1 = DemoAsyncExecutor(4);
        f3_executor_s1_s1.AddFinishedCallback(
                [](AsyncExecutor* executor) {
                    auto results_count = executor->GetResultsCount();
                    std::cout << "> f3_s1_s1 executor result count: " << results_count << std::endl;

                    auto v1 = executor->template GetResult<int>(0);
                    std::cout << "> f3_s1_s1 result 1: " << v1 << std::endl;

                    auto v2 = executor->template GetResult<float>(1);
                    std::cout << "> f3_s1_s1 result 2: " << v2 << std::endl;

                    auto v3 = executor->template GetResult<const char*>(2);
                    std::cout << "> f3_s1_s1 result 3: " << v3 << std::endl;
                });
        FlowNode f3_node_s1_s1("f3_node_s1_s1", &af_3, &f3_executor_s1_s1);
        f3_node_s1.AddNode(&f3_node_s1_s1);

    af_3.Run();
#endif

#ifdef WITH_PERIODIC_TIMER
    PeriodicTimer periodic_timer(TimeVal(1, 0), &PeriodicTimerHandler);
    periodic_timer.Start();
#endif

    EV_Singleton->StartLoop();

    return 0;
}
