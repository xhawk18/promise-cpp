#ifndef INC_PROMISE_CPP_
#define INC_PROMISE_CPP_

#include "promise.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

namespace promise {

PROMISE_API static void healthyCheck(int line, PromiseHolder *promiseHolder) {
#ifndef NDEBUG
    if (!promiseHolder) {
        fprintf(stderr, "line = %d, %d, promiseHolder is null\n", line, __LINE__);
        throw std::runtime_error("");
    }

    for (const auto &owner_ : promiseHolder->owners_) {
        auto owner = owner_.lock();
        if (owner && owner->promiseHolder_.get() != promiseHolder) {
            fprintf(stderr, "line = %d, %d, owner->promiseHolder_ = %p, promiseHolder = %p\n",
                line, __LINE__,
                owner->promiseHolder_.get(),
                promiseHolder);
            throw std::runtime_error("");
        }
    }

    for (const std::shared_ptr<Task> &task : promiseHolder->pendingTasks_) {
        if (!task) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task is null\n", line, __LINE__, promiseHolder);
            throw std::runtime_error("");
        }
        if (task->state_ != TaskState::kPending) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task = %p, task->state_ = %d\n", line, __LINE__,
                promiseHolder, task.get(), (int)task->state_);
            throw std::runtime_error("");
        }
        if (task->promiseHolder_.lock().get() != promiseHolder) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task = %p, task->promiseHolder_ = %p\n", line, __LINE__,
                promiseHolder, task.get(), task->promiseHolder_.lock().get());
            throw std::runtime_error("");
        }
    }
#endif
}

void Promise::dump() const {
#ifndef NDEBUG
    printf("Promise = %p, SharedPromise = %p\n", this, this->sharedPromise_.get());
    if (this->sharedPromise_)
        this->sharedPromise_->dump();
#endif
}

void SharedPromise::dump() const {
#ifndef NDEBUG
    printf("SharedPromise = %p, PromiseHolder = %p\n", this, this->promiseHolder_.get());
    if (this->promiseHolder_)
        this->promiseHolder_->dump();
#endif
}

void PromiseHolder::dump() const {
#ifndef NDEBUG
    printf("PromiseHolder = %p, owners = %d, pendingTasks = %d\n", this, (int)this->owners_.size(), (int)this->pendingTasks_.size());
    for (const auto &owner_ : owners_) {
        auto owner = owner_.lock();
        printf("  owner = %p\n", owner.get());
    }
    for (const auto &task : pendingTasks_) {
        if (task) {
            auto promiseHolder = task->promiseHolder_.lock();
            printf("  task = %p, PromiseHolder = %p\n", task.get(), promiseHolder.get());
        }
        else {
            printf("  task = %p\n", task.get());
        }
    }
#endif
}

PROMISE_API static void join(std::shared_ptr<SharedPromise> &left, const std::shared_ptr<PromiseHolder> &right) {
    healthyCheck(__LINE__, left->promiseHolder_.get());
    healthyCheck(__LINE__, right.get());
    //left->dump();
    //right->dump();

    for (const std::shared_ptr<Task> &task : right->pendingTasks_) {
        task->promiseHolder_ = left->promiseHolder_;
    }
    left->promiseHolder_->pendingTasks_.splice(left->promiseHolder_->pendingTasks_.end(), right->pendingTasks_);

    std::list<std::weak_ptr<SharedPromise>> owners;
    owners.splice(owners.end(), right->owners_);
    right->state_ = TaskState::kResolved;

    if(owners.size() > 100) {
        fprintf(stderr, "Maybe memory leak, too many promise owners: %d", (int)owners.size());
    }

    for (const std::weak_ptr<SharedPromise> &owner_ : owners) {
        std::shared_ptr<SharedPromise> owner = owner_.lock();
        if (owner) {
            owner->promiseHolder_ = left->promiseHolder_;
            left->promiseHolder_->owners_.push_back(owner);
        }
    }

    //left->dump();
    //right->dump();
    //fprintf(stderr, "left->promiseHolder_->owners_ size = %d\n", (int)left->promiseHolder_->owners_.size());


    healthyCheck(__LINE__, left->promiseHolder_.get());
    healthyCheck(__LINE__, right.get());
}

PROMISE_API static void call(std::shared_ptr<Task> task) {
    std::shared_ptr<PromiseHolder> promiseHolder; //Can hold the temporarily created promise
    while (true) {
        if (task->state_ != TaskState::kPending) return;

        promiseHolder = task->promiseHolder_.lock();
        if (promiseHolder->state_ == TaskState::kPending) return;
        task->state_ = promiseHolder->state_;

        std::list<std::shared_ptr<Task>> &pendingTasks = promiseHolder->pendingTasks_;
        //promiseHolder->dump();
        assert(pendingTasks.front() == task);
        pendingTasks.pop_front();
        //promiseHolder->dump();

        try {
            if (promiseHolder->state_ == TaskState::kResolved) {
                if (task->onResolved_.empty()
                    || task->onResolved_.type() == typeid(std::nullptr_t)) {
                    //to next resolved task
                }
                else {
                    const any &value = task->onResolved_.call(promiseHolder->value_);
                    if (value.type() != typeid(Promise)) {
                        promiseHolder->value_ = value;
                        promiseHolder->state_ = TaskState::kResolved;
                    }
                    else {
                        // join the promise
                        Promise promise = value.cast<Promise>();
                        join(promise.sharedPromise_, promiseHolder);
                        promiseHolder = promise.sharedPromise_->promiseHolder_;
                    }
                }
            }
            else if (promiseHolder->state_ == TaskState::kRejected) {
                if (task->onRejected_.empty()
                    || task->onRejected_.type() == typeid(std::nullptr_t)) {
                    //to next rejected task
                    //promiseHolder->value_ = promiseHolder->value_;
                    //promiseHolder->state_ = TaskState::kRejected;
                }
                else {
                    try {
                        const any &value = task->onRejected_.call(promiseHolder->value_);
                        if (value.type() != typeid(Promise)) {
                            promiseHolder->value_ = value;
                            promiseHolder->state_ = TaskState::kResolved;
                        }
                        else {
                            // join the promise
                            Promise promise = value.cast<Promise>();
                            join(promise.sharedPromise_, promiseHolder);
                            promiseHolder = promise.sharedPromise_->promiseHolder_;
                        }
                    }
                    catch (const bad_any_cast &) {
                        //just go through if argument type is not match
                    }
                }
            }
        }
        catch (...) {
            promiseHolder->value_ = std::current_exception();
            promiseHolder->state_ = TaskState::kRejected;
        }

        // get next
        std::list<std::shared_ptr<Task>> &pendingTasks2 = promiseHolder->pendingTasks_;
        if (pendingTasks2.size() == 0) {
            return;
        }

        task = pendingTasks2.front();
    }
}

Defer::Defer(const std::shared_ptr<Task> &task)
    : task_(task)
    , sharedPromise_(std::shared_ptr<SharedPromise>(new SharedPromise{ task->promiseHolder_.lock() })) {
}

void Defer::resolve(const any &arg) const {
    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise_->promiseHolder_;
    promiseHolder->state_ = TaskState::kResolved;
    promiseHolder->value_ = arg;
    call(task_);
}

void Defer::reject(const any &arg) const {
    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise_->promiseHolder_;
    promiseHolder->state_ = TaskState::kRejected;
    promiseHolder->value_ = arg;
    call(task_);
}


Promise Defer::getPromise() const {
    return Promise{ sharedPromise_ };
}


struct DoBreakTag {};

DeferLoop::DeferLoop(const Defer &defer)
    : defer_(defer) {
}

void DeferLoop::doContinue() const {
    defer_.resolve();
}

void DeferLoop::doBreak(const any &arg) const {
    defer_.reject(DoBreakTag(), arg);
}

void DeferLoop::reject(const any &arg) const {
    defer_.reject(arg);
}

Promise DeferLoop::getPromise() const {
    return defer_.getPromise();
}


PromiseHolder::~PromiseHolder() {
    if (this->state_ == TaskState::kRejected) {
        PromiseHolder::onUncaughtException(this->value_);
    }
}

any *PromiseHolder::getUncaughtExceptionHandler() {
    static any onUncaughtException;
    return &onUncaughtException;
}

void PromiseHolder::onUncaughtException(const any &arg) {
    any *onUncaughtException = getUncaughtExceptionHandler();
    if (onUncaughtException != nullptr && !onUncaughtException->empty()) {
        try {
            onUncaughtException->call(reject(arg));
        }
        catch (...) {
            // std::rethrow_exception(std::current_exception());
            fprintf(stderr, "onUncaughtException in line %d\n", __LINE__);
        }
    }
    else {
        //throw arg;
        fprintf(stderr, "onUncaughtException in line %d\n", __LINE__);
    }
}

void PromiseHolder::handleUncaughtException(const any &onUncaughtException) {
    (*getUncaughtExceptionHandler()) = onUncaughtException;
}


Promise &Promise::then(const any &deferOrPromiseOrOnResolved) {
    if (deferOrPromiseOrOnResolved.type() == typeid(Defer)) {
        Defer &defer = deferOrPromiseOrOnResolved.cast<Defer &>();
        return then([defer](const any &arg) -> any {
            defer.resolve(arg);
            return nullptr;
        }, [defer](const any &arg) ->any {
            defer.reject(arg);
            return nullptr;
        });
    }
    else if (deferOrPromiseOrOnResolved.type() == typeid(DeferLoop)) {
        DeferLoop &loop = deferOrPromiseOrOnResolved.cast<DeferLoop &>();
        return then([loop](const any &arg) -> any {
            (void)arg;
            loop.doContinue();
            return nullptr;
        }, [loop](const any &arg) ->any {
            loop.reject(arg);
            return nullptr;
        });
    }
    else if (deferOrPromiseOrOnResolved.type() == typeid(Promise)) {
        Promise &promise = deferOrPromiseOrOnResolved.cast<Promise &>();
        if (promise.sharedPromise_ && promise.sharedPromise_->promiseHolder_) {
            join(this->sharedPromise_, promise.sharedPromise_->promiseHolder_);
            if (this->sharedPromise_->promiseHolder_->pendingTasks_.size() > 0) {
                std::shared_ptr<Task> task = this->sharedPromise_->promiseHolder_->pendingTasks_.front();
                call(task);
            }
        }
        return *this;
    }
    else {
        return then(deferOrPromiseOrOnResolved, any());
    }
}

Promise &Promise::then(const any &onResolved, const any &onRejected) {
    std::shared_ptr<Task> task = std::make_shared<Task>(Task {
        TaskState::kPending,
        sharedPromise_->promiseHolder_,
        onResolved,
        onRejected
    });
    sharedPromise_->promiseHolder_->pendingTasks_.push_back(task);
    call(task);
    return *this;
}

Promise &Promise::fail(const any &onRejected) {
    return then(any(), onRejected);
}

Promise &Promise::always(const any &onAlways) {
    return then(onAlways, onAlways);
}

Promise &Promise::finally(const any &onFinally) {
    return then([onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Defer &defer) {
            try {
                onFinally.call(arg);
            }
            catch (bad_any_cast &) {}
            defer.resolve(arg);
        });
    }, [onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Defer &defer) {
            try {
                onFinally.call(arg);
            }
            catch (bad_any_cast &) {}
            defer.reject(arg);
        });
    });
}


void Promise::resolve(const any &arg) const {
    std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promiseHolder_->pendingTasks_;
    if (pendingTasks_.size() > 0) {
        std::shared_ptr<Task> &task = pendingTasks_.front();
        Defer defer(task);
        defer.resolve(arg);
    }
}

void Promise::reject(const any &arg) const {
    std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promiseHolder_->pendingTasks_;
    if (pendingTasks_.size() > 0) {
        std::shared_ptr<Task> &task = pendingTasks_.front();
        Defer defer(task);
        defer.reject(arg);
    }
}

void Promise::clear() {
    sharedPromise_.reset();
}

Promise::operator bool() const {
    return sharedPromise_.operator bool();
}

Promise newPromise(const std::function<void(Defer &defer)> &run) {
    Promise promise;
    promise.sharedPromise_ = std::make_shared<SharedPromise>();
    promise.sharedPromise_->promiseHolder_ = std::make_shared<PromiseHolder>();
    promise.sharedPromise_->promiseHolder_->owners_.push_back(promise.sharedPromise_);
    
    auto returnAsIs = [](const any &arg) -> any {
        return arg;
    };
    promise.then(returnAsIs);
    std::shared_ptr<Task> &task = promise.sharedPromise_->promiseHolder_->pendingTasks_.front();

    Defer defer(task);
    try {
        run(defer);
    }
    catch (...) {
        defer.reject(std::current_exception());
    }
   
    return promise;
}

Promise newPromise() {
    Promise promise;
    promise.sharedPromise_ = std::make_shared<SharedPromise>();
    promise.sharedPromise_->promiseHolder_ = std::make_shared<PromiseHolder>();
    promise.sharedPromise_->promiseHolder_->owners_.push_back(promise.sharedPromise_);

    auto returnAsIs = [](const any &arg) -> any {
        return arg;
    };
    promise.then(returnAsIs);
    return promise;
}

Promise doWhile(const std::function<void(DeferLoop &loop)> &run) {

    return newPromise([run](Defer &defer) {
        DeferLoop loop(defer);
        run(loop);
    }).then([run](const any &arg) -> any {
        (void)arg;
        return doWhile(run);
    }, [](const any &arg) -> any {
        return newPromise([arg](Defer &defer) {
            //printf("arg. type = %s\n", arg.type().name());

            bool isBreak = false;
            if (arg.type() == typeid(std::vector<any>)) {
                std::vector<any> &args = any_cast<std::vector<any> &>(arg);
                if (args.size() == 2
                    && args[0].type() == typeid(DoBreakTag)
                    && args[1].type() == typeid(std::vector<any>)) {
                    isBreak = true;
                    defer.resolve(args[1]);
                }
            }
            
            if(!isBreak) {
                defer.reject(arg);
            }
        });
    });
}

#if 0
Promise reject(const any &arg) {
    return newPromise([arg](Defer &defer) { defer.reject(arg); });
}

Promise resolve(const any &arg) {
    return newPromise([arg](Defer &defer) { defer.resolve(arg); });
}
#endif

Promise all(const std::list<Promise> &promise_list) {
    if (promise_list.size() == 0) {
        return resolve();
    }

    std::shared_ptr<size_t> finished = std::make_shared<size_t>(0);
    std::shared_ptr<size_t> size = std::make_shared<size_t>(promise_list.size());
    std::shared_ptr<std::vector<any>> retArr = std::make_shared<std::vector<any>>();
    retArr->resize(*size);

    return newPromise([=](Defer &defer) {
        size_t index = 0;
        for (auto promise : promise_list) {
            promise.then([=](const any &arg) {
                (*retArr)[index] = arg;
                if (++(*finished) >= *size) {
                    defer.resolve(*retArr);
                }
            }, [=](const any &arg) {
                defer.reject(arg);
            });

            ++index;
        }
    });
}

Promise race(const std::list<Promise> &promise_list) {
    return newPromise([=](Defer &defer) {
        for (auto promise : promise_list) {
            promise.then([=](const any &arg) {
                defer.resolve(arg);
            }, [=](const any &arg) {
                defer.reject(arg);
            });
        }
    });
}

Promise raceAndReject(const std::list<Promise> &promise_list) {
    return race(promise_list).finally([promise_list] {
        for (auto promise : promise_list) {
            promise.reject();
        }
    });
}

Promise raceAndResolve(const std::list<Promise> &promise_list) {
    return race(promise_list).finally([promise_list] {
        for (auto promise : promise_list) {
            promise.resolve();
        }
    });
}

 
} // namespace promise

#endif
