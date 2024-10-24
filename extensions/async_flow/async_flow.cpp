#include "async_flow.h"
#include "flow_node.h"
#include "flow_scheduler.h"

namespace evt_loop {

State AsyncFlow::Run() {
    entering_times_++;
    printf("[AsyncFlow::Run->enter] name: %s, times: %d, state: %d\n",
            name_.c_str(), entering_times_, (int)state_);

    if (! root_->IsFinished()) {
        state_ = root_->Run();
    }
    if (root_->IsFinished()) {
        state_ = root_->GetState();
        OnFinished();
    }

    printf("[AsyncFlow::Run->return] name: %s, state: %d\n", name_.c_str(), (int)state_);
    return state_;
}

void AsyncFlow::Resume() {
    printf("[AsyncFlow::Resume] name: %s\n", name_.c_str());
    Run();
    if (IsFinished() && scheduler_) {
        scheduler_->RemoveFlow(name_);
        /*
        scheduler_->MarkRemovingFlow(name_);
        scheduler_->ClearRemovedFlows();
        */
    }
}

void AsyncFlow::OnFinished() {
    printf("[AsyncFlow::OnFinished] name: %s, times: %d, state: %d\n",
            name_.c_str(), entering_times_, (int)state_);

    if (finished_cb_) {
        finished_cb_(this);
    }
}

const std::vector<any>*
AsyncFlow::GetResults() const {
    auto executor = GetLastFinishedExecutor();
    return executor ? &(executor->GetResults()) : nullptr;
}

}  // ns evt_loop
