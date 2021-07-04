#include "promise.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

namespace promise {

void healthyCheck(SharedPromise *sharedPromise) {
    if (!sharedPromise->promise_) {
        fprintf(stderr, "line = %d, SharedPromise = %p, promise_ is null\n", __LINE__, sharedPromise);
        throw std::runtime_error("");
    }

    for (const std::shared_ptr<Task> &task : sharedPromise->promise_->pendingTasks_) {
        if (!task) {
            fprintf(stderr, "line = %d, SharedPromise = %p, promise_ = %p, task is null\n", __LINE__,
                sharedPromise, sharedPromise->promise_.get());
            throw std::runtime_error("");
        }
        if (task->state_ != TaskState::kPending) {
            fprintf(stderr, "line = %d, SharedPromise = %p, promise_ = %p, task = %p, task->state_ = %d\n", __LINE__,
                sharedPromise, sharedPromise->promise_.get(), task.get(), (int)task->state_);
            throw std::runtime_error("");
        }
        if (task->sharedPromise_.lock()->promise_ != sharedPromise->promise_) {
            fprintf(stderr, "line = %d, SharedPromise = %p, promise_ = %p, task = %p, task->sharedPromise_ = %p, promise = %p\n", __LINE__,
                sharedPromise, sharedPromise->promise_.get(), task.get(), task->sharedPromise_.lock().get(), task->sharedPromise_.lock()->promise_.get());
            throw std::runtime_error("");
        }

        if (task->sharedPromise_.lock().get() != sharedPromise) {
            fprintf(stderr, "line = %d, SharedPromise = %p, promise_ = %p, task = %p, task->sharedPromise_ = %p\n", __LINE__,
                sharedPromise, sharedPromise->promise_.get(), task.get(), task->sharedPromise_.lock().get());
            throw std::runtime_error("");
        }
    }
}

void healthyCheck(Defer *defer) {
    if (!defer->sharedPromise_) {
        fprintf(stderr, "line = %d, Defer = %p, sharedPromise_ is null\n", __LINE__, defer);
        throw std::runtime_error("");
    }
    healthyCheck(defer->sharedPromise_.get());
}



static void join(std::shared_ptr<SharedPromise> &left, std::shared_ptr<SharedPromise> &right) {
    // healthyCheck(left.get());
    // healthyCheck(right.get());

    for (const std::shared_ptr<Task> &task : right->promise_->pendingTasks_) {
        task->sharedPromise_ = left;
    }
    left->promise_->pendingTasks_.splice(left->promise_->pendingTasks_.end(), right->promise_->pendingTasks_);
    right->promise_ = left->promise_;

    // healthyCheck(left.get());
    // right must not be healthy now, so do not use healthyCheck(right.get());
}

static void call(std::shared_ptr<Task> task) {
    std::shared_ptr<SharedPromise> sharedPromiseHolder; //Holder for temporarily created promise

    while (true) {
        if (task->state_ != TaskState::kPending) return;

        std::shared_ptr<SharedPromise> sharedPromise = task->sharedPromise_.lock();
        std::shared_ptr<Promise> promise = sharedPromise->promise_;
        if (promise->state_ == TaskState::kPending) return;
        task->state_ = promise->state_;

        std::list<std::shared_ptr<Task>> &pendingTasks = promise->pendingTasks_;
        assert(pendingTasks.front() == task);
        pendingTasks.pop_front();

        try {
            if (promise->state_ == TaskState::kResolved) {
                const any &value = task->onResolved_.call(promise->value_);
                if (value.type() != typeid(Defer)) {
                    promise->value_ = value;
                    promise->state_ = TaskState::kResolved;
                }
                else {
                    // join the promise
                    Defer defer = value.cast<Defer>();
                    join(defer.sharedPromise_, sharedPromise);
                    sharedPromiseHolder = defer.sharedPromise_;
                    promise = defer.sharedPromise_->promise_;
                }
            }
            else if (promise->state_ == TaskState::kRejected) {
                if (task->onRejected_.empty()) {
                    //to next rejected task
                    //promise->value_ = promise->value_;
                    //promise->state_ = TaskState::kRejected;
                }
                else {
                    const any &value = task->onRejected_.call(promise->value_);
                    if (value.type() != typeid(Defer)) {
                        promise->value_ = value;
                        promise->state_ = TaskState::kResolved;
                    }
                    else {
                        // join the promise
                        Defer defer = value.cast<Defer>();
                        join(defer.sharedPromise_, sharedPromise);
                        sharedPromiseHolder = defer.sharedPromise_;
                        promise = defer.sharedPromise_->promise_;
                    }
                }
            }
        }
        catch (...) {
            promise->value_ = std::current_exception();
            promise->state_ = TaskState::kRejected;
        }

        // get next
        std::list<std::shared_ptr<Task>> &pendingTasks2 = promise->pendingTasks_;
        if (pendingTasks2.size() == 0) return;
        task = pendingTasks2.front();
    }
}

Callback::Callback(const std::shared_ptr<Task> &task)
    : task_(task)
    , sharedPromise_(task->sharedPromise_.lock()) {
}

void Callback::resolve(const any &arg) const {
    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<Promise> promise = sharedPromise_->promise_;
    promise->state_ = TaskState::kResolved;
    promise->value_ = arg;
    call(task_);
}

void Callback::reject(const any &arg) const {
    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<Promise> promise = sharedPromise_->promise_;
    promise->state_ = TaskState::kRejected;
    promise->value_ = arg;
    call(task_);
}

void Callback::resolve() const {
    resolve(nullptr);
}
void Callback::reject() const {
    reject(nullptr);
}


struct DoBreakTag {};

LoopCallback::LoopCallback(const Callback &cb)
    : cb_(cb) {
}

void LoopCallback::doContinue() const {
    cb_.resolve();
}

void LoopCallback::doBreak(const any &arg) const {
    cb_.reject(std::pair<DoBreakTag, any>(DoBreakTag(), arg));
}

void LoopCallback::reject(const any &arg) const {
    cb_.reject(arg);
}

void LoopCallback::doBreak() const {
    cb_.reject(std::pair<DoBreakTag, any>(DoBreakTag(), nullptr));
}

void LoopCallback::reject() const {
    cb_.reject();
}

Defer &Defer::then(const any &callbackOrOnResolved) {
    if (callbackOrOnResolved.type() == typeid(Callback)) {
        Callback &cb = callbackOrOnResolved.cast<Callback>();
        return then([cb](const any &arg) -> any {
            cb.resolve(arg);
            return nullptr;
        }, [cb](const any &arg) ->any {
            cb.reject(arg);
            return nullptr;
        });
    }
    else if (callbackOrOnResolved.type() == typeid(LoopCallback)) {
        LoopCallback &cb = callbackOrOnResolved.cast<LoopCallback>();
        return then([cb](const any &arg) -> any {
            (void)arg;
            cb.doContinue();
            return nullptr;
        }, [cb](const any &arg) ->any {
            cb.reject(arg);
            return nullptr;
        });
    }
    else {
        return then(callbackOrOnResolved, nullptr);
    }
}

Defer &Defer::then(const any &onResolved, const any &onRejected) {
    std::shared_ptr<Task> task = std::make_shared<Task>(Task {
        TaskState::kPending,
        sharedPromise_,
        onResolved,
        onRejected
    });
    sharedPromise_->promise_->pendingTasks_.push_back(task);
    call(task);
    return *this;
}

Defer &Defer::fail(const any &onRejected) {
    return then([](const any &arg) -> any {
        return arg;
    }, onRejected);
}

Defer &Defer::always(const any &onAlways) {
    return then(onAlways, onAlways);
}

Defer &Defer::finally(const any &onFinally) {
    return then([onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Callback &cb) {
            onFinally.call(arg);
            cb.resolve(arg);
        });
    }, [onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Callback &cb) {
            onFinally.call(arg);
            cb.reject(arg);
        });
    });
}


void Defer::resolve(const any &arg) const {
    std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promise_->pendingTasks_;
    if (pendingTasks_.size() > 0) {
        std::shared_ptr<Task> &task = pendingTasks_.front();
        Callback cb(task);
        cb.resolve(arg);
    }
}

void Defer::reject(const any &arg) const {
    std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promise_->pendingTasks_;
    if (pendingTasks_.size() > 0) {
        std::shared_ptr<Task> &task = pendingTasks_.front();
        Callback cb(task);
        cb.reject(arg);
    }
}

void Defer::resolve() const {
    resolve(nullptr);
}

void Defer::reject() const {
    reject(nullptr);
}

Defer newPromise(const std::function<void(Callback &cb)> &run) {
    Defer defer;
    defer.sharedPromise_ = std::make_shared<SharedPromise>();
    defer.sharedPromise_->promise_ = std::make_shared<Promise>();
    
    auto returnAsIs = [](const any &arg) -> any {
        return arg;
    };
    defer.then(returnAsIs);
    std::shared_ptr<Task> &task = defer.sharedPromise_->promise_->pendingTasks_.front();

    Callback cb(task);
    try {
        run(cb);
    }
    catch (...) {
        cb.reject(std::current_exception());
    }
   
    return defer;
}

Defer doWhile(const std::function<void(LoopCallback &cb)> &run) {

    return newPromise([run](Callback &cb) {
        LoopCallback loopCb(cb);
        run(loopCb);
    }).then([run](const any &arg) -> any {
        (void)arg;
        return doWhile(run);
    }, [](const any &arg) -> any {
        return newPromise([arg](Callback &cb) {
            if (arg.type() == typeid(std::pair<DoBreakTag, any>)) {
                const any &result = arg.cast<std::pair<DoBreakTag, any>>().second;
                cb.resolve(result);
            }
            else {
                cb.reject(arg);
            }
        });
    });
}

Defer reject(const any &arg) {
    return newPromise([arg](Callback &cb) { cb.reject(arg); });
}

Defer resolve(const any &arg) {
    return newPromise([arg](Callback &cb) { cb.resolve(arg); });
}

Defer reject() {
    return newPromise([](Callback &cb) { cb.reject(nullptr); });
}

Defer resolve() {
    return newPromise([](Callback &cb) { cb.resolve(nullptr); });
}

Defer all(const std::initializer_list<Defer> &promise_list) {
    if (promise_list.size() == 0) {
        return resolve();
    }

    std::shared_ptr<size_t> finished = std::make_shared<size_t>(0);
    std::shared_ptr<size_t> size = std::make_shared<size_t>(promise_list.size());
    std::shared_ptr<std::vector<any>> retArr = std::make_shared<std::vector<any>>();
    retArr->resize(*size);

    return newPromise([=](Callback &cb) {
        size_t index = 0;
        for (auto defer : promise_list) {
            defer.then([=](const any &arg) {
                (*retArr)[index] = arg;
                if (++(*finished) >= *size) {
                    cb.resolve(*retArr);
                }
            }, [=](const any &arg) {
                cb.reject(arg);
            });

            ++index;
        }
    });
}

Defer race(const std::initializer_list<Defer> &promise_list) {
    return newPromise([=](Callback &cb) {
        for (auto defer : promise_list) {
            defer.then([=](const any &arg) {
                cb.resolve(arg);
            }, [=](const any &arg) {
                cb.reject(arg);
            });
        }
    });
}

Defer raceAndReject(const std::initializer_list<Defer> &promise_list) {
    std::vector<Defer> copy_list = promise_list;
    return race(promise_list).finally([copy_list] {
        for (auto defer : copy_list) {
            defer.reject();
        }
    });
}

Defer raceAndResolve(const std::initializer_list<Defer> &promise_list) {
    std::vector<Defer> copy_list = promise_list;
    return race(promise_list).finally([copy_list] {
        for (auto defer : copy_list) {
            defer.resolve();
        }
    });
}

 
} // namespace promise

