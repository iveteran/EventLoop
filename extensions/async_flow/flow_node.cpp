#include "flow_node.h"
#include "async_executor.h"
#include "async_flow.h"

namespace evt_loop {

State FlowNode::Run() {
    entering_times_++;
    flow_->SetCurrentNode(this);

    printf("[FlowNode::Run()->enter] name: %s, times: %d, state: %d\n",
            name_.c_str(), entering_times_, (int)state_);

    if (IsFinished()) {
        goto _fn_exit;
    }
    state_ = State::Running;

    if (executor_ && !executor_->IsFinished()) {
        state_ = executor_->Run();
        if (state_ == State::Suspended) {
            goto _fn_exit;
        } else if (state_ == State::Exception) {
            auto [ errcode, errmsg ] = executor_->GetError();
            errcode_ = errcode;
            errmsg_ = errmsg;
            goto _fn_exit;
        }
    }

    //printf("[FlowNode::Run] name: %s, current_node_index_: %ld, nodes size: %ld\n",
    //      name_.c_str(), current_node_index_, nodes_.size());
    for (; current_node_index_ < nodes_.size(); ++current_node_index_) {
        // 如果当前节点是Suspended的，不执行字节点
        if (state_ == State::Suspended) {
            goto _fn_exit;
        }
        auto node = nodes_[current_node_index_];
        if (node->IsFinished()) {
            continue;
        }

        state_ = node->Run();

        if (state_ == State::Suspended) {
            goto _fn_exit;
        }
    }

    //printf("[FlowNode::Run] name: %s, current_node_index_: %ld, nodes size: %ld, state: %d\n",
    //        name_.c_str(), current_node_index_, nodes_.size(), (int)state_);
    if (current_node_index_ == nodes_.size() && state_ != State::Suspended) {
        state_ = State::Done;
        OnFinished();
    }

_fn_exit:
    printf("[FlowNode::Run()->return] name: %s, state: %d\n", name_.c_str(), (int)state_);
    return state_;
}

State FlowNode::Resume() {
    printf("[FlowNode::Resume] name: %s, times: %d, state: %d\n",
            name_.c_str(), entering_times_, (int)state_);

    Run();
    if (IsFinished()) {
        if (parent_) {
            parent_->Resume();
        } else {
            // Is flow root
            flow_->Resume();
        }
    }
    return state_;
}

void FlowNode::OnFinished() {
    printf("[FlowNode::OnFinished] name: %s, times: %d, state: %d\n",
            name_.c_str(), entering_times_, (int)state_);

    if (finished_cb_) {
        finished_cb_(this);
    }
}

void FlowNode::OnExecutorSuccess(const AsyncExecutor* executor) {
    if (executor->IsSuccess()) {
        flow_->AddFinishedExecutor(executor);
    }
}

} // ns evt_loop
