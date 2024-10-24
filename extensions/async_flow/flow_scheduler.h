#ifndef _FLOW_SCHEDULER_H
#define _FLOW_SCHEDULER_H

#include <string>
#include <map>
#include <set>

using std::string;
using std::map;
using std::set;

namespace evt_loop {

class AsyncFlow;

class FlowScheduler
{
    public:
    void AddFlow(const string& name, AsyncFlow* flow);
    void AddFlow(AsyncFlow* flow);
    void MarkRemovingFlow(const string& name);
    void RemoveFlow(const string& name);
    void ClearRemovedFlows();

    int Run();

    private:
    map<string, AsyncFlow*> flows_;
    set<string> removed_flows_;
};

}  // ns evt_loop

#endif  // _FLOW_SCHEDULER_H
