#ifndef _ASYNC_FLOW_H
#define _ASYNC_FLOW_H

#include <string>
#include <map>
#include <vector>
#include <any>
#include <functional>
#include "flow_state.h"

using std::string;
using std::map;
using std::any;

namespace evt_loop {

class FlowNode;
class FlowScheduler;
class AsyncExecutor;

class AsyncFlow
{
    public:
    //void AddNode(FlowNode* node);
    //void InsertNode(int pos, FlowNode* node);

    using FinishedCallback = std::function<void (AsyncFlow*)>;

    AsyncFlow(const char* name) : name_(name)
    {
    }
    ~AsyncFlow()
    {
        printf("[AsyncFlow::~AsyncFlow]\n");
    }

    void SetFinishedCallback(const FinishedCallback& cb) {
        finished_cb_ = cb;
    }

    const string& GetName() const { return name_; }

    void AttachScheduler(FlowScheduler* scheduler) { scheduler_ = scheduler; }
    FlowScheduler* GetScheduler() { return scheduler_; }

    void SetFlowRoot(FlowNode* node) {
        root_ = node;
    }

    void SetCurrentNode(FlowNode* node) {
        current_node_ = node;
    }

    State Run();

    void Suspend() {
        state_ = State::Suspended;
    }

    void Resume();

    void OnFinished();

    bool IsFinished() const {
        return state_ == State::Done || state_ == State::Exception;
    }

    template<typename T>
    void AddWhiteboardValue(const string& name, const T& value)
    {
        whiteboard_[name] = value;
    }

    template<typename T>
    const T* GetWhiteboardValue(const string& name) const
    {
        auto iter = whiteboard_.find(name);
        return iter != whiteboard_.end() ? std::any_cast<const T*>(&(iter->second)) : nullptr;
    }

    void AddFinishedExecutor(const AsyncExecutor* executor) {
        finished_executors_.push_back(executor);
    }

    const AsyncExecutor* GetLastFinishedExecutor() const {
        return finished_executors_.empty() ? nullptr : *(finished_executors_.rbegin());
    }

    const std::vector<any>* GetResults() const;

    private:
    string name_;
    int entering_times_ = 0;
    FlowNode* root_ = nullptr;
    FlowNode* current_node_ = nullptr;

    FlowScheduler* scheduler_ = nullptr;
    State state_ = State::Ready;

    map<string, std::any> whiteboard_;

    std::vector<const AsyncExecutor*> finished_executors_;
    FinishedCallback finished_cb_;
};

}  // ns evt_loop

#endif  // _ASYNC_FLOW_H
