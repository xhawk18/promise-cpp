#pragma once
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_

#ifdef PROMISE_HEADONLY
#define PROMISE_API inline
#else
#define PROMISE_API
#endif

#include <list>
#include <vector>
#include <memory>
#include <functional>
#include "any.hpp"

namespace promise {

enum TaskState {
    kPending,
    kResolved,
    kRejected
};

struct PromiseHolder;
struct SharedPromise;
struct Promise;

struct Task {
    TaskState state_;
    std::weak_ptr<PromiseHolder> promiseHolder_;
    any                          onResolved_;
    any                          onRejected_;
};


/* 
 * Task state in TaskList always be kPending
 */
struct PromiseHolder {
    PROMISE_API ~PromiseHolder();
    std::list<std::weak_ptr<SharedPromise>> owners_;
    std::list<std::shared_ptr<Task>>        pendingTasks_;
    TaskState                               state_;
    any                                     value_;

    PROMISE_API void dump() const;
    PROMISE_API static any *getUncaughtExceptionHandler();
    PROMISE_API static void onUncaughtException(const any &arg);
    PROMISE_API static void handleUncaughtException(const any &onUncaughtException);
};

// Check if ...ARGS only has one any type
template<typename ...ARGS>
struct is_one_any : public std::is_same<typename tuple_remove_cvref<std::tuple<ARGS...>>::type, std::tuple<any>> {
};

struct SharedPromise {
    std::shared_ptr<PromiseHolder> promiseHolder_;
    PROMISE_API void dump() const;
};

struct Defer {
    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void resolve(ARGS &&...args) const {
        resolve(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(ARGS &&...args) const {
        reject(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    PROMISE_API void resolve(const any &arg) const;
    PROMISE_API void reject(const any &arg) const;

    PROMISE_API Promise getPromise() const;

private:
    friend struct Promise;
    friend Promise newPromise(const std::function<void(Defer &defer)> &run);
    PROMISE_API Defer(const std::shared_ptr<Task> &task);
    std::shared_ptr<Task>          task_;
    std::shared_ptr<SharedPromise> sharedPromise_;
};

struct DeferLoop {
    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void doBreak(ARGS &&...args) const {
        doBreak(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(ARGS &&...args) const {
        reject(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    PROMISE_API void doContinue() const;
    PROMISE_API void doBreak(const any &arg) const;
    PROMISE_API void reject(const any &arg) const;


    PROMISE_API Promise getPromise() const;

private:
    friend Promise doWhile(const std::function<void(DeferLoop &loop)> &run);
    PROMISE_API DeferLoop(const Defer &cb);
    Defer defer_;
};

struct Promise {
    PROMISE_API Promise &then(const any &deferOrOnResolved);
    PROMISE_API Promise &then(const any &onResolved, const any &onRejected);
    PROMISE_API Promise &fail(const any &onRejected);
    PROMISE_API Promise &always(const any &onAlways);
    PROMISE_API Promise &finally(const any &onFinally);

    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void resolve(ARGS &&...args) const {
        resolve(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(ARGS &&...args) const {
        reject(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    PROMISE_API void resolve(const any &arg) const;
    PROMISE_API void reject(const any &arg) const;

    PROMISE_API void clear();
    PROMISE_API operator bool() const;

    PROMISE_API void dump() const;

    std::shared_ptr<SharedPromise> sharedPromise_;
};


PROMISE_API Promise newPromise(const std::function<void(Defer &defer)> &run);
PROMISE_API Promise doWhile(const std::function<void(DeferLoop &loop)> &run);
template<typename ...ARGS>
inline Promise resolve(ARGS &&...args) {
    return newPromise([&args...](Defer &defer) { defer.resolve(std::forward<ARGS>(args)...); });
}

template<typename ...ARGS>
inline Promise reject(ARGS &&...args) {
    return newPromise([&args...](Defer &defer) { defer.reject(std::forward<ARGS>(args)...); });
}


/* Returns a promise that resolves when all of the promises in the iterable
   argument have resolved, or rejects with the reason of the first passed
   promise that rejects. */
PROMISE_API Promise all(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise all(const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return all(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise all(PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return all(std::list<Promise>{ defer0, promise_list ... });
}


/* returns a promise that resolves or rejects as soon as one of
the promises in the iterable resolves or rejects, with the value
or reason from that promise. */
PROMISE_API Promise race(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise race(const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return race(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise race(PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return race(std::list<Promise>{ defer0, promise_list ... });
}


PROMISE_API Promise raceAndReject(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise raceAndReject(const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return raceAndReject(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise raceAndReject(PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return raceAndReject(std::list<Promise>{ defer0, promise_list ... });
}


PROMISE_API Promise raceAndResolve(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise raceAndResolve(const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return raceAndResolve(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise raceAndResolve(PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return raceAndResolve(std::list<Promise>{ defer0, promise_list ... });
}

inline void handleUncaughtException(const any &onUncaughtException) {
    PromiseHolder::handleUncaughtException(onUncaughtException);
}

} // namespace promise

#ifdef PROMISE_HEADONLY
#include "../src/promise.cpp"
#endif

#endif
