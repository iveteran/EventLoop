#ifndef _TEMPLATED_ASYNC_EXECUTOR_H
#define _TEMPLATED_ASYNC_EXECUTOR_H

#include <vector>
#include "async_executor.h"

using std::vector;

namespace evt_loop {

class TemplatedAsyncExecutor : public AsyncExecutor {
    public:
    template<typename T>
    void SetCallee(const T& fn) {
        fn_ = fn;
    }

    template<typename T>
    void AddParam(const T& v) {
        params_.push_back(v);
    }

    template<typename T>
    const T& GetParam(int idx) const {
        assert(idx < params_.size());
        return std::any_cast<const T&>(params_[idx]);
    }

    size_t GetParamSize() const {
        return params_.size();
    }

    template<typename F, typename... Args>
    State Run(F fn, Args... args) {
        printf("[AsyncExecutor::Run(fn, ...args)] node: %s\n", name_.c_str());

        state_ = State::Running;
        fn(args...);
        state_ = State::Suspended;

        return state_;
    }

    State Run() {
        /*
        if (params_.size() == 1) {
            Run(fn_, params_[0]);
        } if (params_.size() == 2) {
            Run(fn_, std::any_cast<int>(params_[0]), std::any_cast<std::vector<std::any>>(params_[1]));
        }
        */
        // TODO
        return state_;
    }

    protected:
    std::any fn_;
    vector<std::any> params_;
};

} // ns evt_loop

#endif  // _TEMPLATED_ASYNC_EXECUTOR_H
