#include "async_executor.h"
#include "flow_node.h"

namespace evt_loop {

void AsyncExecutor::AttachToNode(FlowNode* node) {
    node_ = node;
    name_ = node_->GetName();
}

State AsyncExecutor::Run() {
    printf("[AsyncExecutor::Run()] node: %s\n", node_->GetName().c_str());

    state_ = State::Done;
    //state_ = State::Exception;

    OnFinished();
    return state_;
}

void AsyncExecutor::OnFinished() {
    printf("[AsyncExecutor::OnFinished()] node: %s, state: %d\n",
            node_->GetName().c_str(), (int)state_);

    for (auto cb : finished_cbs_) {
        cb(this);
    }
}

void AsyncExecutor::OnError(int errcode, const char* errmsg) {
    printf("[AsyncExecutor::OnError] errcode %d, errmsg: %s\n", errcode, errmsg);
    state_ = State::Exception;
    OnFinished();
}

}  // ns evt_loop
