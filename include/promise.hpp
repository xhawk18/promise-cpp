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
    Defer &fail(const any &onRejected);
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



/* Returns a promise that resolves when all of the promises in the iterable
   argument have resolved, or rejects with the reason of the first passed
   promise that rejects. */
Defer all(const std::initializer_list<Defer> &promise_list);
template <typename ... PROMISE_LIST>
inline Defer all(PROMISE_LIST ...promise_list) {
    return all({ promise_list ... });
}


/* returns a promise that resolves or rejects as soon as one of
the promises in the iterable resolves or rejects, with the value
or reason from that promise. */
Defer race(const std::initializer_list<Defer> &promise_list);
template <typename ... PROMISE_LIST>
inline Defer race(PROMISE_LIST ...promise_list) {
    return race({ promise_list ... });
}

Defer raceAndReject(const std::initializer_list<Defer> &promise_list);
template <typename ... PROMISE_LIST>
inline Defer raceAndReject(PROMISE_LIST ...promise_list) {
    return raceAndReject({ promise_list ... });
}

Defer raceAndResolve(const std::initializer_list<Defer> &promise_list);
template <typename ... PROMISE_LIST>
inline Defer raceAndResolve(PROMISE_LIST ...promise_list) {
    return raceAndReject({ promise_list ... });
}


} // namespace promise
#endif
