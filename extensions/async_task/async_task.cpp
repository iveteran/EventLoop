#include "async_task.h"
#include "eventloop/logger.h"
#include <sys/eventfd.h>
//#include <pthread.h>

namespace evt_loop {

void* TaskEntry(void* userdata)
{
    el_logger->debug("[TaskEntry] enter task thread");
    auto async_task = (AsyncTask*)userdata;

    if (async_task->task_fn_) {
        el_logger->debug("[TaskEntry] enter user task");
        async_task->result_ = async_task->task_fn_(async_task->args_);
        el_logger->debug("[TaskEntry] user task finished");
    }

    uint64_t v = 1;
    int ret = write(async_task->efd_, &v, sizeof(v));
    if (ret != sizeof(v)) {
        async_task->SetErrorMessage("failed to write eventfd");
        el_logger->error("[TaskEntry] {}", async_task->errmsg_);
    }

    el_logger->debug("[TaskEntry] exit task thread");
    return nullptr;
}

AsyncTask::AsyncTask() : task_fn_(nullptr), args_(nullptr)
{
    efd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd_ < 0) {
        el_logger->error("[AsyncTask::AsyncTask] eventfd create: {}", strerror(errno));
    } else {
        SetFD(efd_);
    }
}

void AsyncTask::AssignTask(const TaskFn& task_fn, void* args, const TaskResultCallback& cb)
{
    task_fn_ = task_fn;
    args_ = args;
    result_cb_ = cb;
}

bool AsyncTask::Run()
{
    if (efd_ < 0) {
        return false;
    }
    el_logger->info("[AsyncTask::Run] ready to create task thread");
    int ret = pthread_create(&tid_, NULL, TaskEntry, this);
    if (ret != 0) {
        SetErrorMessage("pthread_create failed");
        el_logger->error("[AsyncTask::Run] {}", errmsg_);
        return false;
    }
    el_logger->info("[AsyncTask::Run] task thread created, thread id: {}", tid_);
    return true;
}

void AsyncTask::OnCompleted()
{
    uint64_t v;
    int ret = read(efd_, &v, sizeof(v));
    if (ret < 0) {
        SetErrorMessage("failed to read eventfd");
        el_logger->error("[AsyncTask::OnCompleted] {}", errmsg_);
        return;
    }
    close(efd_);

    if (result_cb_) {
        result_cb_(result_);
    }
}

void AsyncTask::OnEvents(uint32_t events)
{
    if (events & FileEvent::READ) {
        el_logger->debug("[AsyncTask::OnEvents] events: {}", events);
        OnCompleted();
    }

    if (events & FileEvent::ERROR) {
        OnError(errno, strerror(errno));
    }
}

void AsyncTask::SetErrorMessage(const char* errmsg)
{
    if (errmsg) {
        char buf[128];
        snprintf(buf, sizeof(buf), "failed to write eventfd, thread id: %ld", tid_);
        errmsg_ = buf;
    } else {
        errmsg_.clear();
    }
}

}  // ns evt_loop
