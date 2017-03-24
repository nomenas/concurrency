//
// Created by Naum Puroski on 22/03/2017.
//

#ifndef TASKCONCEPT_TASK_H
#define TASKCONCEPT_TASK_H

#include "TaskExecutor.h"
#include "Utils.h"

#include <functional>
#include <vector>
#include <numeric>
#include <future>
#include <utility>

class Task {
public:
    template <typename T>
    using Callback = std::function<void(T*)>;

    explicit Task(Callback<Task> callback) : _callback(std::move(callback)) {}

    virtual ~Task() {
        if (_executor) {
            _executor->cancel(this);
        }
    }

    void set_executor(TaskExecutor* executor) {
        _executor = executor;
    }

    // run task and provide callback that would be called when tasks finishes
    Task& run(bool force_synchronous_mode = false) {
        if (_executor && !force_synchronous_mode) {
            _executor->execute(this);
        } else {
            execute();
        }

        return *this;
    }

    // run task and wait for results
    Task& wait() {
        safe_call([this](){_promise.get_future().wait();});
        return *this;
    }

    template <typename T>
    const T& get_results() {
        return *static_cast<T*>(&wait());
    }

protected:

    virtual void execute() = 0;

    template<typename T, typename... Args>
    Task& create_task(Args... args) {
        std::unique_ptr<T> sub_task{new T{args...}};
        sub_task->set_executor(_executor);
        Task& return_value = *sub_task;
        _sub_tasks.push_back(std::move(sub_task));
        return return_value;
    }

    void mark_as_done() {
        _promise.set_value();
        if (_callback) {
            _callback(this);
        }
    }

private:
    Callback<Task> _callback;
    std::promise<void> _promise;
    std::vector<std::unique_ptr<Task>> _sub_tasks;
    TaskExecutor* _executor = nullptr;
};

template <typename T>
Task::Callback<Task> create_task_callback(Task::Callback<T> callback) {
    return [callback](Task* task){if (callback) {callback(static_cast<T*>(task));}};
}

#endif //TASKCONCEPT_TASK_H
