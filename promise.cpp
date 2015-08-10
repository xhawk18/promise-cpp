
#include <functional>
#include <memory>
#include <cassert>
#include <stdio.h>

#define PTI do{printf("%d, %s\n", __LINE__, __func__);} while(0)

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
        //PTI;
    }
    
    virtual ~PromiseEx() {
        //PTI;
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
        //PTI;
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
        //PTI;
    }
    
    virtual ~Promise() {
        //printf("this = %p ", this);
        //PTI;
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
        //PTI;
        try {
            func(d);
            return;
        } catch(...) {}
        d->reject();
    }
    
    Defer call_next() {
        if(!next_.get()) {
            PTI;
        }
        else if(!is_called_ && is_resolved_) {
            PTI;
            is_called_ = true;
            Defer d = next_->call_resolve(next_);
            if(d.get())
                d->call_next();
            return d;
        }
        else if(!is_called_ && is_rejected_) {
            PTI;
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
            PTI;
            printf("%p\n", self->next_.get());
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

int main(int argc, char **argv) {
    Defer d1;
    
    newPromise([argc, &d1](Defer d){
        PTI;
        d1 = d;
        d->resolve();
        //d->reject();
    })->then([](){
        PTI;
        Defer d1 = newPromise([](Defer d){
            PTI;
        //d->resolve();
        });
        d1->resolve();
        throw 33;
        return d1;        
        //return d1;        
    }, [&d1](){
        PTI;
        throw 3;
    })->then([](){
        PTI;
    })->then([](){
        PTI;
    })->fail([](){
        PTI;
    })->always([](){
        PTI;
    });
    
    
    printf("later=================\n");
    //(d1)->resolve();
    
    return 0;
}
