#include "promise.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

namespace promise {

void healthyCheck(int line, SharedPromise *sharedPromise, bool isCheckShared) {
    if (!sharedPromise->promiseHolder_) {
        fprintf(stderr, "line = %d, %d, SharedPromise = %p, promiseHolder_ is null\n", line, __LINE__, sharedPromise);
        throw std::runtime_error("");
    }

    for (const auto &owner_ : sharedPromise->promiseHolder_->owners_) {
        auto owner = owner_.lock();
        if (owner && owner->promiseHolder_ != sharedPromise->promiseHolder_) {
            fprintf(stderr, "line = %d, %d, SharedPromise = %p, owner->promiseHolder_ = %p, sharedPromise->promiseHolder_ = %p\n",
                line, __LINE__,
                sharedPromise,
                owner->promiseHolder_.get(),
                sharedPromise->promiseHolder_.get());
            throw std::runtime_error("");
        }
    }

    for (const std::shared_ptr<Task> &task : sharedPromise->promiseHolder_->pendingTasks_) {
        if (!task) {
            fprintf(stderr, "line = %d, %d, SharedPromise = %p, promiseHolder_ = %p, task is null\n", line, __LINE__,
                sharedPromise, sharedPromise->promiseHolder_.get());
            throw std::runtime_error("");
        }
        if (task->state_ != TaskState::kPending) {
            fprintf(stderr, "line = %d, %d, SharedPromise = %p, promiseHolder_ = %p, task = %p, task->state_ = %d\n", line, __LINE__,
                sharedPromise, sharedPromise->promiseHolder_.get(), task.get(), (int)task->state_);
            throw std::runtime_error("");
        }
        if (task->sharedPromise_.lock()->promiseHolder_ != sharedPromise->promiseHolder_) {
            fprintf(stderr, "line = %d, %d, SharedPromise = %p, promiseHolder_ = %p, task = %p, task->sharedPromise_ = %p, promise = %p\n", line, __LINE__,
                sharedPromise, sharedPromise->promiseHolder_.get(), task.get(), task->sharedPromise_.lock().get(), task->sharedPromise_.lock()->promiseHolder_.get());
            throw std::runtime_error("");
        }

        if (isCheckShared && task->sharedPromise_.lock().get() != sharedPromise) {
            fprintf(stderr, "line = %d, %d, SharedPromise = %p, promiseHolder_ = %p, task = %p, task->sharedPromise_ = %p\n", line, __LINE__,
                sharedPromise, sharedPromise->promiseHolder_.get(), task.get(), task->sharedPromise_.lock().get());
            throw std::runtime_error("");
        }
    }
}

void healthyCheck(int line, Promise *promise, bool isCheckShared) {
    if (!promise->sharedPromise_) {
        fprintf(stderr, "line = %d, %d, Promise = %p, sharedPromise_ is null\n", line, __LINE__, promise);
        throw std::runtime_error("");
    }
    healthyCheck(line, promise->sharedPromise_.get(), isCheckShared);
}



static void join(std::shared_ptr<SharedPromise> &left, std::shared_ptr<SharedPromise> &right) {
    //healthyCheck(__LINE__, left.get(), true);
    //healthyCheck(__LINE__, right.get(), true);

    for (const std::shared_ptr<Task> &task : right->promiseHolder_->pendingTasks_) {
        task->sharedPromise_ = left;
    }
    left->promiseHolder_->pendingTasks_.splice(left->promiseHolder_->pendingTasks_.end(), right->promiseHolder_->pendingTasks_);

    std::list<std::weak_ptr<SharedPromise>> owners;
    owners.splice(owners.end(), right->promiseHolder_->owners_);

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

    //fprintf(stderr, "left->promiseHolder_->owners_ size = %d\n", (int)left->promiseHolder_->owners_.size());


    //healthyCheck(__LINE__, left.get(), true);
    //healthyCheck(__LINE__, right.get(), false);
    // right must not be healthy now, so do not use healthyCheck(right.get());
}

static void call(std::shared_ptr<Task> task) {
    //std::shared_ptr<SharedPromise> sharedPromiseHolder; //Holder for temporarily created promise

    while (true) {
        if (task->state_ != TaskState::kPending) return;

        std::shared_ptr<SharedPromise> sharedPromise = task->sharedPromise_.lock();
        std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise->promiseHolder_;
        if (promiseHolder->state_ == TaskState::kPending) return;
        task->state_ = promiseHolder->state_;

        std::list<std::shared_ptr<Task>> &pendingTasks = promiseHolder->pendingTasks_;
        assert(pendingTasks.front() == task);
        pendingTasks.pop_front();

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
                        join(promise.sharedPromise_, sharedPromise);
                        //sharedPromiseHolder = promise.sharedPromise_;
                        assert(promiseHolder == promise.sharedPromise_->promiseHolder_);
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
                    const any &value = task->onRejected_.call(promiseHolder->value_);
                    if (value.type() != typeid(Promise)) {
                        promiseHolder->value_ = value;
                        promiseHolder->state_ = TaskState::kResolved;
                    }
                    else {
                        // join the promise
                        Promise promise = value.cast<Promise>();
                        join(promise.sharedPromise_, sharedPromise);
                        //sharedPromiseHolder = promise.sharedPromise_;
                        assert(promiseHolder == promise.sharedPromise_->promiseHolder_);
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
        if (pendingTasks2.size() == 0) return;
        task = pendingTasks2.front();
    }
}

Defer::Defer(const std::shared_ptr<Task> &task)
    : task_(task)
    , sharedPromise_(task->sharedPromise_.lock()) {
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


Promise &Promise::then(const any &callbackOrOnResolved) {
    if (callbackOrOnResolved.type() == typeid(Defer)) {
        Defer &defer = callbackOrOnResolved.cast<Defer &>();
        return then([defer](const any &arg) -> any {
            defer.resolve(arg);
            return nullptr;
        }, [defer](const any &arg) ->any {
            defer.reject(arg);
            return nullptr;
        });
    }
    else if (callbackOrOnResolved.type() == typeid(DeferLoop)) {
        DeferLoop &loop = callbackOrOnResolved.cast<DeferLoop &>();
        return then([loop](const any &arg) -> any {
            (void)arg;
            loop.doContinue();
            return nullptr;
        }, [loop](const any &arg) ->any {
            loop.reject(arg);
            return nullptr;
        });
    }
    else {
        return then(callbackOrOnResolved, any());
    }
}

Promise &Promise::then(const any &onResolved, const any &onRejected) {
    std::shared_ptr<Task> task = std::make_shared<Task>(Task {
        TaskState::kPending,
        sharedPromise_,
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
            onFinally.call(arg);
            defer.resolve(arg);
        });
    }, [onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Defer &defer) {
            onFinally.call(arg);
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

Promise doWhile(const std::function<void(DeferLoop &loop)> &run) {

    return newPromise([run](Defer &defer) {
        DeferLoop loop(defer);
        run(loop);
    }).then([run](const any &arg) -> any {
        (void)arg;
        return doWhile(run);
    }, [](const any &arg) -> any {
        return newPromise([arg](Defer &defer) {
            if (arg.type() == typeid(std::tuple<DoBreakTag, any>)) {
                std::tuple<DoBreakTag, any> &args = arg.cast<std::tuple<DoBreakTag, any> &>();
                const any &result = std::get<1>(args);
                defer.resolve(result);
            }
            else {
                defer.reject(arg);
            }
        });
    });
}

Promise reject(const any &arg) {
    return newPromise([arg](Defer &defer) { defer.reject(arg); });
}

Promise resolve(const any &arg) {
    return newPromise([arg](Defer &defer) { defer.resolve(arg); });
}

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

