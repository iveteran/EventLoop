#ifndef _ASYNC_EXECUTOR_EXAMPLES_H
#define _ASYNC_EXECUTOR_EXAMPLES_H

#include "flow_node.h"
#include "async_flow.h"
//#include "templated_async_executor.h"
#include "eventloop/timer_handler.h"
#include <iostream>

using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3;
using namespace evt_loop;

class DatabaseAsyncQuerier
{
    public:
    struct ResultRow
    {
        int value_int;
        float value_float;
        const char* value_str;
    };

    using ResultCallback = std::function<void (int, const char*, const ResultRow*)>;

    DatabaseAsyncQuerier(const ResultCallback& result_cb=nullptr, int wait_sec=5)
    {
        result_cb_ = result_cb;
        async_simulator_ = new OneshotTimer(
                TimeVal(wait_sec, 0),
                std::bind(
                    &DatabaseAsyncQuerier::OnTimerTriggered,
                    this,
                    std::placeholders::_1
                    )
                );
    }
    void SetResultCallback(const ResultCallback& cb)
    {
        result_cb_ = cb;
    }

    template<typename... Args>
    bool Query(const char* sql, Args... args)
    {
        async_simulator_->Start();
        return true;
    }

    protected:
    void OnTimerTriggered(TimerEvent* timer)
    {
        printf("[DatabaseAsyncQuerier::OnTimerTriggered] triggered\n");
        ResultRow db_row { 100, 11.22, "dummy data" };
        OnDatabaseResult(0, "Success", &db_row);
    }
    void OnDatabaseResult(int errcode, const char* errmsg, const ResultRow* data)
    {
        if (result_cb_) {
            result_cb_(errcode, errmsg, data);
        }
    }

    private:
    void* db_conn_;
    ResultCallback result_cb_;
    OneshotTimer *async_simulator_;
};

class DatabaseAsyncExecutor : public AsyncExecutor
{
    public:
    DatabaseAsyncExecutor()
    {
        db_querier_.SetResultCallback(std::bind(&DatabaseAsyncExecutor::OnQueryResult, this, _1, _2, _3));
    }

    virtual State Run() override
    {
        printf("[DatabaseAsyncExecutor::Run()] node: %s, enter state: %d\n", node_->GetName().c_str(), (int)state_);

        const char* sql = "select * from tbl_dummy";
        db_querier_.Query(sql, 1, 12.34, "hello");

        state_ = State::Suspended;
        printf("[DatabaseAsyncExecutor::Run()] node: %s, exit state: %d\n", node_->GetName().c_str(), (int)state_);
        return state_;
    }

    protected:
    void OnQueryResult(int errcode, const char* errmsg, const DatabaseAsyncQuerier::ResultRow* data)
    {
        printf("[DatabaseAsyncExecutor::OnQueryResult] triggered, node: %s\n", node_->GetName().c_str());
        state_ = State::Done;
        if (errcode != 0) {
            OnError(errcode, errmsg);
        } else {
            OnResult(data->value_int, data->value_float, data->value_str);
        }
        OnFinished();
        node_->Resume();
    }

    private:
    DatabaseAsyncQuerier db_querier_;
};

class DemoAsyncExecutor : public AsyncExecutor
{
    public:
    DemoAsyncExecutor(int wait_sec=5) : async_simulator_(TimeVal(wait_sec, 0), std::bind(&DemoAsyncExecutor::OnAsyncResult, this, std::placeholders::_1))
    {
        wait_sec_ = wait_sec;
    }

    virtual State Run() override
    {
        //AsyncExecutor::Run();
        printf("[DemoAsyncExecutor::Run()] node: %s, enter state: %d\n", node_->GetName().c_str(), (int)state_);

        auto previous_executor = node_->GetFlow()->GetLastFinishedExecutor();
        if (previous_executor) {
            auto results = previous_executor->GetResults();
            auto results_count = results.size();
            std::cout << "> previous results count: " << results_count << std::endl;
            if (results_count == 3) {
                auto v1 = std::any_cast<int>(results[0]);
                std::cout << "> previous result 1: " << v1 << std::endl;

                auto v2 = std::any_cast<float>(results[1]);
                std::cout << "> previous result 2: " << v2 << std::endl;

                auto v3 = std::any_cast<const char*>(results[2]);
                std::cout << "> previous result 3: " << v3 << std::endl;
            }
        }

        async_simulator_.Start();
        printf("[DemoAsyncExecutor::Run] async_simulator started, wait seconds: %d\n", wait_sec_);

        state_ = State::Suspended;
        printf("[DemoAsyncExecutor::Run()] node: %s, exit state: %d\n", node_->GetName().c_str(), (int)state_);
        return state_;
    }

    protected:
    void OnAsyncResult(TimerEvent* timer)
    {
        printf("[DemoAsyncExecutor::OnAsyncResult] triggered, node: %s\n", node_->GetName().c_str());
        state_ = State::Done;
        OnResult(1, 1.234f, "foo bar");
        OnFinished();
        node_->Resume();
    }

    private:
    int wait_sec_;
    OneshotTimer async_simulator_;
};

#endif  // _ASYNC_EXECUTOR_EXAMPLES_H
