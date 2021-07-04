#pragma once
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_

#include <list>
#include <memory>
#include <functional>
#include "any.hpp"

namespace promise {


enum TaskState {
    kPending,
    kResolved,
    kRejected
};

struct Promise;
struct SharedPromise;
struct Defer;
struct Task {
    TaskState state_;
    std::weak_ptr<SharedPromise> sharedPromise_;
    any                          onResolved_;
    any                          onRejected_;
};


/* 
 * Task state in TaskList always be kPending
 */
struct Promise {
    std::list<std::shared_ptr<Task>> pendingTasks_;
    TaskState                        state_;
    any                              value_;
};

struct SharedPromise {
    std::shared_ptr<Promise> promise_;
};

struct Callback {
    Callback(const std::shared_ptr<Task> &task);
    void resolve(const any &arg) const;
    void reject(const any &arg) const;
    void resolve() const;
    void reject() const;
private:
    std::shared_ptr<Task>          task_;
    std::shared_ptr<SharedPromise> sharedPromise_;
};

struct LoopCallback {
    LoopCallback(const Callback &cb);
    void doContinue() const;
    void doBreak(const any &) const;
    void reject(const any &) const;
    void doBreak() const;
    void reject() const;
private:
    Callback cb_;
};

struct Defer {
    Defer &then(const any &callbackOrOnResolved);
    Defer &then(const any &onResolved, const any &onRejected);
    Defer &always(const any &onAlways);
    Defer &finally(const any &onFinally);

    void resolve(const any &arg) const;
    void reject(const any &arg) const;
    void resolve() const;
    void reject() const;

    std::shared_ptr<SharedPromise> sharedPromise_;
};


Defer newPromise(const std::function<void(Callback &cb)> &run);
Defer doWhile(const std::function<void(LoopCallback &cb)> &run);
Defer reject(const any &arg);
Defer resolve(const any &arg);
Defer reject();
Defer resolve();


} // namespace promise
#endif
