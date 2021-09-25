#include <stdio.h>
#include <string>
#include "promise.hpp"
using namespace promise;



int main() {
    Promise a0;
    a0.sharedPromise_ = std::make_shared<SharedPromise>();
    a0.sharedPromise_->promiseHolder_ = std::make_shared<PromiseHolder>();
    Promise a1 = a0;
    
    printf("%p %p, %p %p\n", a0.sharedPromise_.get(), (a0.sharedPromise_->promiseHolder_).get(),
                             a1.sharedPromise_.get(), (a1.sharedPromise_->promiseHolder_).get());
    
    
    Promise b0;
    b0.sharedPromise_ = std::make_shared<SharedPromise>();
    b0.sharedPromise_->promiseHolder_ = std::make_shared<PromiseHolder>();
    Promise b1 = b0;

    printf("%p %p, %p %p\n", b0.sharedPromise_.get(), (b0.sharedPromise_->promiseHolder_).get(),
                             b1.sharedPromise_.get(), (b1.sharedPromise_->promiseHolder_).get());


    b0.sharedPromise_->promiseHolder_ = a0.sharedPromise_->promiseHolder_;
    printf("%p %p, %p %p\n", b0.sharedPromise_.get(), (b0.sharedPromise_->promiseHolder_).get(),
                             b1.sharedPromise_.get(), (b1.sharedPromise_->promiseHolder_).get());



    Promise a = newPromise([](Defer &defer) {
        defer.reject(15);
        defer.resolve(13);
    })/*/.then([](const any &x) {
        printf("resolved xx = %d\n", x.cast<int>());
    }, [](const any &x) {
        printf("rejected xx = %d\n", x.cast<int>());
        //throw 18;
        return 18;
    })*/;

    Promise b = a;
    Promise c = b;

    resolve(3, 4, std::string("ss"));

    newPromise([](Defer &defer) {
        defer.resolve(nullptr);
    }).then([=](const any &) {
        //console.log(a)
        printf("a = ?\n");
        return c;
    }).then([=](const any &x) {
        printf("resolved x = %d\n", x.cast<int>());
        return a;
    }, [=](const any &x) {
        printf("rejected x = %d\n", x.cast<int>());
        return a;
    }).finally([](const any &x) {
        printf("finally x = %d\n", x.cast<int>());
        //return x;
    }).then([](const any &x) {
        printf("pre finally resolved\n");
        printf("pre finally resolved x = %d\n", x.cast<int>());
    }, [](const any &x) {
        printf("pre finally rejected\n");
        printf("pre finally rejected x = %d\n", x.cast<int>());
    }).finally([]() {
        printf("finally\n");
    });

    return 0;
}
