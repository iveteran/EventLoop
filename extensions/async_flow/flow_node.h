#ifndef _FLOW_NODE_H
#define _FLOW_NODE_H

#include "async_executor.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

using std::string;
using std::vector;
using std::map;

namespace evt_loop {

class AsyncFlow;
class AsyncExecutor;

class FlowNode {
    public:
    using FinishedCallback = std::function<void (FlowNode*)>;

    FlowNode(const char* name, AsyncFlow* flow, AsyncExecutor* executor=nullptr)
        : name_(name), executor_(executor), flow_(flow)
    {
        if (executor_) {
            executor_->AttachToNode(this);
            executor_->AddFinishedCallback(std::bind(&FlowNode::OnExecutorSuccess, this, std::placeholders::_1));
        }
    }

    void SetFinishedCallback(const FinishedCallback& cb) {
        finished_cb_ = cb;
    }

    const string& GetName() const { return name_; }

    State GetState() const { return state_; }

    void AddNode(FlowNode* node) {
        node->Attach(this);
        nodes_.push_back(node);
        auto node_name = node->GetName();
        if (! node_name.empty()) {
            nodes_map_[node_name] = node;
        }
    }
    void Attach(FlowNode* parent) {
        parent_ = parent;
    }

    FlowNode* GetSubNode(int idx) const {
        return (size_t)idx < nodes_.size() ? nodes_[idx] : nullptr;
    }
    FlowNode* GetSubNode(const string& name) const {
        auto iter = nodes_map_.find(name);
        return iter != nodes_map_.end() ? iter->second : nullptr;
    }
    FlowNode* GetParentNode() const {
        return parent_;
    }
    FlowNode* GerPreviousNode() const {
        if (!parent_) {
            return nullptr;
        }
        if (parent_->GetCurrentNodeIndex() > 0) {
            parent_->GetSubNode(parent_->GetCurrentNodeIndex() - 1);
        } else {
            parent_->GetParentNode();
        }
    }
    size_t GetCurrentNodeIndex() const {
        return current_node_index_;
    }

    State Run();

    State Resume();

    void Suspend() {
        state_ = State::Suspended;
    }

    bool IsFinished() const {
        return state_ == State::Done || state_ == State::Exception;
    }

    const AsyncFlow* GetFlow() const {
        return flow_;
    }

    const std::vector<any>* GetResults() const {
        return executor_ ? &(executor_->GetResults()) : nullptr;
    }

    std::pair<int, string>
    GetError() const {
        return { errcode_, errmsg_ };
    }

    protected:
    void OnFinished();
    void OnExecutorSuccess(const AsyncExecutor* executor);

    private:
    string name_;
    int entering_times_ = 0;
    State state_ = State::Ready;

    AsyncExecutor* executor_ = nullptr;
    int errcode_ = 0;
    string errmsg_;

    AsyncFlow* flow_ = nullptr;
    FlowNode* parent_ = nullptr;
    vector<FlowNode*> nodes_;
    map<string, FlowNode*> nodes_map_;
    size_t current_node_index_ = 0;

    FinishedCallback finished_cb_;
};

}  // ns evt_loop

#endif  // _FLOW_NODE_H
