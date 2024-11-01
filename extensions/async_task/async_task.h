#ifndef _ASYNC_TASK_H
#define _ASYNC_TASK_H

#include "eventloop/io_event.h"
#include <memory>
#include <string>

using std::string;

namespace evt_loop {

void* TaskEntry(void* userdata);

class AsyncTask : public IOEvent
{
    friend void* TaskEntry(void* userdata);

    public:
    using TaskFn = std::function<void* (void* args)>;
    using TaskResultCallback = std::function<void (void* result)>;

    AsyncTask();
    void AssignTask(const TaskFn& task_fn, void* args, const TaskResultCallback& cb);
    bool Run();

    protected:
    void OnEvents(uint32_t events, void* ctx = nullptr) override;
    void OnCompleted();
    void SetErrorMessage(const char* errmsg);

    private:
    pthread_t   tid_;
    int         efd_;
    string      errmsg_;

    TaskFn task_fn_;
    TaskResultCallback result_cb_;

    void* args_;
    void* result_;
};
using AsyncTaskUptr = std::unique_ptr<AsyncTask>;

}  // ns evt_loop

#endif  // _ASYNC_TASK_H
