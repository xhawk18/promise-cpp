#pragma once
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_

#if defined PROMISE_HEADONLY
#define PROMISE_API inline
#elif defined PROMISE_BUILD_SHARED

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(promise_EXPORTS) // add by CMake 
#    ifdef __GNUC__
#      define  PROMISE_API __attribute__(dllexport)
#    else
#      define  PROMISE_API __declspec(dllexport)
#    endif
#  else
#    ifdef __GNUC__
#      define  PROMISE_API __attribute__(dllimport)
#    else
#      define  PROMISE_API __declspec(dllimport)
#    endif
#  endif // promise_EXPORTS

#elif defined __GNUC__
#  if __GNUC__ >= 4
#    define PROMISE_API __attribute__ ((visibility ("default")))
#  else
#    define PROMISE_API
#  endif

#elif defined __clang__
#  define PROMISE_API __attribute__ ((visibility ("default")))
#else
#   error "Do not know how to export classes for this platform"
#endif

#else
#define PROMISE_API
#endif

#ifndef PROMISE_MULTITHREAD
#   define PROMISE_MULTITHREAD 1
#endif


#include <list>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "any.hpp"

namespace promise {

enum class TaskState {
    kPending,
    kResolved,
    kRejected
};

struct PromiseHolder;
struct SharedPromise;
class Promise;

// Source code location, for debug
struct Loc {
    int line_;
    const char *file_;
};

struct CallRecord {
    Loc loc_;
    int serialNo_;
    std::chrono::system_clock::time_point callTime_;
};

#define PM_MAX_LOC 100
#define PM_LOC promise::Loc{__LINE__, __FILE__}

struct CallStack {
    std::list<CallRecord> *locations_;
    PROMISE_API void dump() const;
};


// Task
struct Task {
    TaskState state_;
    std::weak_ptr<PromiseHolder> promiseHolder_;
    Loc                          loc_;
    any                          onResolved_;
    any                          onRejected_;
};

#if PROMISE_MULTITHREAD
struct Mutex {
public:
    PROMISE_API Mutex();
    PROMISE_API void lock();
    PROMISE_API void unlock();
    PROMISE_API void lock(size_t lock_count);
    PROMISE_API void unlock(size_t lock_count);
    inline size_t lock_count() const { return lock_count_; }
    std::condition_variable_any cond_;
private:
    std::recursive_mutex mutex_;
    size_t lock_count_;
};
#endif

/* 
 * Task state in TaskList always be kPending
 */
struct PromiseHolder {
    PROMISE_API PromiseHolder();
    PROMISE_API ~PromiseHolder();
    std::list<std::weak_ptr<SharedPromise>> owners_;
    std::list<std::shared_ptr<Task>>        pendingTasks_;
    std::list<CallRecord>                   callStack_;
    TaskState                               state_;
    any                                     value_;
#if PROMISE_MULTITHREAD
    std::shared_ptr<Mutex>                  mutex_;
#endif

    PROMISE_API void dump() const;
    PROMISE_API static any *getUncaughtExceptionHandler();
    PROMISE_API static any *getDefaultUncaughtExceptionHandler();
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
    PROMISE_API CallStack callStack() const;
#if PROMISE_MULTITHREAD
    PROMISE_API std::shared_ptr<Mutex> obtainLock() const;
#endif
};

class Defer {
public:
    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void resolve(const Loc &loc, ARGS &&...args) const {
        resolve(loc, any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(const Loc &loc, ARGS &&...args) const {
        reject(loc, any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    PROMISE_API void resolve(const Loc &loc, const any &arg) const;
    PROMISE_API void reject(const Loc &loc, const any &arg) const;

    PROMISE_API Promise getPromise() const;
    PROMISE_API CallStack callStack() const;

private:
    friend class Promise;
    friend PROMISE_API Promise newPromise(const Loc &loc, const std::function<void(Defer &defer)> &run);
    PROMISE_API Defer(const std::shared_ptr<Task> &task);
    std::shared_ptr<Task>          task_;
    std::shared_ptr<SharedPromise> sharedPromise_;
};

class DeferLoop {
public:
    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void doBreak(const Loc &loc, ARGS &&...args) const {
        doBreak(loc, any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(const Loc &loc, ARGS &&...args) const {
        reject(loc, any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    PROMISE_API void doContinue(const Loc &loc) const;
    PROMISE_API void doBreak(const Loc &loc, const any &arg) const;
    PROMISE_API void reject(const Loc &loc, const any &arg) const;


    PROMISE_API Promise getPromise() const;
    PROMISE_API CallStack callStack() const;

private:
    friend PROMISE_API Promise doWhile(const Loc &loc, const std::function<void(DeferLoop &loop)> &run);
    PROMISE_API DeferLoop(const Defer &cb);
    Defer defer_;
};

class Promise {
public:
    PROMISE_API Promise &then(const Loc &loc, const any &deferOrPromiseOrOnResolved);
    PROMISE_API Promise &then(const Loc &loc, const any &onResolved, const any &onRejected);
    PROMISE_API Promise &fail(const Loc &loc, const any &onRejected);
    PROMISE_API Promise &always(const Loc &loc, const any &onAlways);
    PROMISE_API Promise &finally(const Loc &loc, const any &onFinally);

    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void resolve(const Loc &loc, ARGS &&...args) const {
        resolve(loc, any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    template<typename ...ARGS,
        typename std::enable_if<!is_one_any<ARGS...>::value>::type *dummy = nullptr>
    inline void reject(const Loc &loc, ARGS &&...args) const {
        reject(loc, any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }

    PROMISE_API void resolve(const Loc &loc, const any &arg) const;
    PROMISE_API void reject(const Loc &loc, const any &arg) const;

    PROMISE_API void clear();
    PROMISE_API operator bool() const;

    PROMISE_API void dump() const;
    PROMISE_API CallStack callStack() const;

    std::shared_ptr<SharedPromise> sharedPromise_;
};


PROMISE_API CallStack callStack();
PROMISE_API Promise newPromise(const Loc &loc, const std::function<void(Defer &defer)> &run);
PROMISE_API Promise newPromise(const Loc &loc);
PROMISE_API Promise doWhile(const Loc &loc, const std::function<void(DeferLoop &loop)> &run);
template<typename ...ARGS>
inline Promise resolve(const Loc &loc, ARGS &&...args) {
    return newPromise(loc, [loc, &args...](Defer &defer) { defer.resolve(loc, std::forward<ARGS>(args)...); });
}

template<typename ...ARGS>
inline Promise reject(const Loc &loc, ARGS &&...args) {
    return newPromise(loc, [loc, &args...](Defer &defer) { defer.reject(loc, std::forward<ARGS>(args)...); });
}


/* Returns a promise that resolves when all of the promises in the iterable
   argument have resolved, or rejects with the reason of the first passed
   promise that rejects. */
PROMISE_API Promise all(const Loc &loc, const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise all(const Loc &loc, const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return all(loc, copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise all(const Loc &loc, PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return all(loc, std::list<Promise>{ defer0, promise_list ... });
}


/* returns a promise that resolves or rejects as soon as one of
the promises in the iterable resolves or rejects, with the value
or reason from that promise. */
PROMISE_API Promise race(const Loc &loc, const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise race(const Loc &loc, const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return race(loc, copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise race(const Loc &loc, PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return race(loc, std::list<Promise>{ defer0, promise_list ... });
}


PROMISE_API Promise raceAndReject(const Loc &loc, const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise raceAndReject(const Loc &loc, const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return raceAndReject(loc, copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise raceAndReject(const Loc &loc, PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return raceAndReject(loc, std::list<Promise>{ defer0, promise_list ... });
}


PROMISE_API Promise raceAndResolve(const Loc &loc, const std::list<Promise> &promise_list);
template<typename PROMISE_LIST,
    typename std::enable_if<is_iterable<PROMISE_LIST>::value
                            && !std::is_same<PROMISE_LIST, std::list<Promise>>::value
    >::type *dummy = nullptr>
inline Promise raceAndResolve(const Loc &loc, const PROMISE_LIST &promise_list) {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return raceAndResolve(loc, copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST, typename std::enable_if<!is_iterable<PROMISE0>::value>::type *dummy = nullptr>
inline Promise raceAndResolve(const Loc &loc, PROMISE0 defer0, PROMISE_LIST ...promise_list) {
    return raceAndResolve(loc, std::list<Promise>{ defer0, promise_list ... });
}

inline void handleUncaughtException(const any &onUncaughtException) {
    PromiseHolder::handleUncaughtException(onUncaughtException);
}

} // namespace promise

#ifdef PROMISE_HEADONLY
#include "promise_inl.hpp"
#endif

#endif
