#include "flow_scheduler.h"
#include "async_flow.h"

namespace evt_loop {

void FlowScheduler::AddFlow(const string& name, AsyncFlow* flow) {
    flow->AttachScheduler(this);
    flows_[name] = flow;
}

void FlowScheduler::AddFlow(AsyncFlow* flow) {
    flow->AttachScheduler(this);
    flows_[flow->GetName()] = flow;
}

void FlowScheduler::MarkRemovingFlow(const string& name) {
    removed_flows_.insert(name);
}

void FlowScheduler::RemoveFlow(const string& name) {
    flows_.erase(name);
    printf("[FlowScheduler::RemoveFlow] flow: %s removed, current flows size: %ld\n", name.c_str(), flows_.size());
}

void FlowScheduler::ClearRemovedFlows() {
    for (auto name : removed_flows_) {
        printf("[FlowScheduler::ClearRemovedFlows] remove flow: %s\n", name.c_str());
        flows_.erase(name);
    }
    printf("[FlowScheduler::ClearRemovedFlows] current flow size: %ld\n", flows_.size());
    removed_flows_.clear();
}

int FlowScheduler::Run() {
    printf("[FlowScheduler::Run] flows size: %ld\n", flows_.size());

    for (auto iter = flows_.begin(); iter != flows_.end(); ++iter) {
        auto name = iter->first;
        auto flow = iter->second;

        flow->Run();

        if (flow->IsFinished()) {
            MarkRemovingFlow(name);
        }
    }
    ClearRemovedFlows();
    return 0;
}

}  // ns evt_loop
