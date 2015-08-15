/*
 * Promise API implemented by cpp as Javascript promise style 
 *
 * Copyright (c) 2012, xhawk18
 * at gmail.com
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_

#include <functional>
#include <memory>

namespace promise {

struct Promise;
typedef std::shared_ptr<Promise> Defer;


typedef void (*FnSimple)();


template <typename RET, typename FUNC>
struct ResolveChecker;
template <typename RET, typename FUNC>
struct RejectChecker;


template <typename Promise, typename FUNC_ON_RESOLVE, typename FUNC_ON_REJECT>
struct PromiseEx 
    : public Promise {
    FUNC_ON_RESOLVE on_resolve_;
    FUNC_ON_REJECT on_reject_;
    
    PromiseEx(FUNC_ON_RESOLVE on_resolve, FUNC_ON_REJECT on_reject)
        : on_resolve_(on_resolve)
        , on_reject_(on_reject) {
    }
    
    virtual ~PromiseEx() {
    }
    
    virtual Defer call_resolve(Defer self) {
        return ResolveChecker<decltype(on_resolve_()), FUNC_ON_RESOLVE>::call(on_resolve_, self);
    }
    virtual Defer call_reject(Defer self) {
        return RejectChecker<decltype(on_reject_()), FUNC_ON_REJECT>::call(on_reject_, self);
    }
};

struct Promise {
private:
    explicit Promise(const Promise &){
    }

public:
    Defer next_;
    bool is_resolved_;
    bool is_rejected_;
    bool is_called_;
    
    explicit Promise()
        : next_(NULL)
        , is_resolved_(false)
        , is_rejected_(false)
        , is_called_(false) {
    }
    
    virtual ~Promise() {
    }
    
    void resolve() {
        if(is_resolved_ || is_rejected_)
            return;
        is_resolved_ = true;
        call_next();
    }
    void reject() {
        if(is_resolved_ || is_rejected_)
            return;
        is_rejected_ = true;
        call_next();
    }

    virtual Defer call_resolve(Defer self) = 0;
    virtual Defer call_reject(Defer self) = 0;

    template <typename FUNC>
    void run(FUNC func, Defer d) {
        try {
            func(d);
            return;
        } catch(...) {}
        d->reject();
    }
    
    Defer call_next() {
        if(!next_.get()) {
        }
        else if(!is_called_ && is_resolved_) {
            is_called_ = true;
            Defer d = next_->call_resolve(next_);
            if(d.get())
                d->call_next();
            return d;
        }
        else if(!is_called_ && is_rejected_) {
            is_called_ = true;
            Defer d =  next_->call_reject(next_);
            if (d.get())
                d->call_next();
            return d;
        }

        return next_;
    }

    template <typename FUNC_ON_RESOLVE, typename FUNC_ON_REJECT>
    Defer then(FUNC_ON_RESOLVE on_resolve, FUNC_ON_REJECT on_reject) {
        Defer promise(new PromiseEx<Promise, FUNC_ON_RESOLVE, FUNC_ON_REJECT>(on_resolve, on_reject));
        next_ = promise;
        return call_next();
    }

    template <typename FUNC_ON_RESOLVE>
    Defer then(FUNC_ON_RESOLVE on_resolve) {
        return then<FUNC_ON_RESOLVE, FnSimple>(on_resolve, NULL);
    }

    template <typename FUNC_ON_REJECT>
    Defer fail(FUNC_ON_REJECT on_reject) {
        return then<FnSimple, FUNC_ON_REJECT>(NULL, on_reject);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) {
        return then<FUNC_ON_ALWAYS, FUNC_ON_ALWAYS>(on_always, on_always);
    }    
};

template <typename RET, typename FUNC>
struct ResolveChecker {
    static Defer call(FUNC func, Defer self) {
        bool success = false;
        try {
            func();
            success = true;
        } catch(...) {}
        if(success) self->resolve();
        else        self->reject();
        return self;
    }
};

template <typename FUNC>
struct ResolveChecker<Defer, FUNC> {
    static Defer call(FUNC func, Defer self) {
        try {
            Defer ret = func();
            ret->next_ = self->next_;
            return ret;
        } catch(...) {}

        self->reject();
        return self;
    }
};

template <>
struct ResolveChecker<void, FnSimple> {
    static Defer call(FnSimple func, Defer self) {
        bool success = true;
        if(func != NULL) {
            try {
                (*func)();
            } catch(...) {
                success = false;
            }
        }
        if(success) self->resolve();
        else        self->reject();
        return self;
    }
};

template <typename RET, typename FUNC>
struct RejectChecker {
    static Defer call(FUNC func, Defer self) {
        bool success = false;
        try {
            func();
            success = true;
        } catch(...) {}
        if(success) self->resolve();
        else        self->reject();
        return self;
    }
};

template <typename FUNC>
struct RejectChecker<Defer, FUNC> {
    static Defer call(FUNC func, Defer self) {
        try {
            Defer ret = func();
            ret->next_ = self->next_;
            return ret;
        } catch(...) {}
        
        self->reject();
        return self;
    }
};


template <>
struct RejectChecker<void, FnSimple> {
    static Defer call(FnSimple func, Defer self) {
        bool success = false;
        if(func != NULL){
            try {
                (*func)();
                success = true;
            } catch(...) {
            }
        }
        if(success)
            self->resolve();
        else
            self->reject();
        return self;
    }
};



template <typename FUNC>
Defer newPromise(FUNC func) {
    Defer promise(new PromiseEx<Promise, FnSimple, FnSimple>(NULL, NULL));
    promise->run(func, promise);
    return promise;
}


}
#endif
