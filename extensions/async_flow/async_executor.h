#ifndef _ASYNC_EXECUTOR_H
#define _ASYNC_EXECUTOR_H

#include <string>
#include <vector>
#include <any>
#include <functional>
#include <cassert>
#include "flow_state.h"

using std::string;
using std::vector;
using std::any;

namespace evt_loop {

class FlowNode;

class AsyncExecutor {
    public:
    using FinishedCallback = std::function<void (AsyncExecutor*)>;

    virtual ~AsyncExecutor() { }
    void AttachToNode(FlowNode* node);

    void AddFinishedCallback(const FinishedCallback& cb) {
        finished_cbs_.push_back(cb);
    }

    virtual State Run();

    State GetState() const { return state_; }

    bool IsFinished() const {
        return state_ == State::Done || state_ == State::Exception;
    }

    bool IsSuccess() const {
        return state_ == State::Done;
    }

    const std::pair<int, string>
    GetError() const {
        return { errcode_, errmsg_ };
    }

    size_t GetResultsCount() const {
        return results_.size();
    }

    const vector<any>& GetResults() const {
        return results_;
    }

    template<typename T>
    const T& GetResult(int idx) {
        assert((size_t)idx < results_.size());
        return any_cast<const T&>(results_[idx]);
    }

    template<typename T, typename... Values>
    void OnResult(const T& value, Values... values) {
        OnResult(value);
        OnResult(values...);
    }
    template<typename T>
    void OnResult(const T& value) {
        results_.push_back(value);
        state_ = State::Done;
    }

    protected:
    virtual void OnFinished();

    void OnError(int errcode, const char* errmsg);

    protected:
    FlowNode* node_ = nullptr;
    State state_ = State::Ready;

    string name_;
    int errcode_ = 0;
    string errmsg_;

    vector<std::any> results_;
    vector<FinishedCallback> finished_cbs_;
};

} // ns evt_loop

#endif  // _ASYNC_EXECUTOR_H
