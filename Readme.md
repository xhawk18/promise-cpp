# C++ promise/A+ library in Javascript styles.

## Usage
   promise-cpp is header only and easy to use, just #include "promise.hpp" is enough.
   With promise-cpp, you can resolve or reject any type of data without writing complex template code.

## Example 

* test/test0.cpp: a simple test code for promise resolve/reject operations.

* test/test_timer.cpp: promisified timer based on asio callback timer. (boost::asio required)

* test/test_http_client.cpp: promisified flow for asynchronized http client. (boost::asio, boost::beast required)

* test/test_http_server.cpp: promisified flow for asynchronized http server. (boost::asio, boost::beast required)

* test/test_tasks.cpp: benchmark test for promisified asynchronized tasks. (boost::asio required)

Please check folder "build" to get the codelite/msvc projects for the test code above.

### Compiler required

The library has passed test on these compilers --

* gcc 5

* Visual studio 2015 sp3

* clang 3.4.2

### Build tips

The example projects use boost::asio as io service, and use boost::beast as http service. 
You need to install [boost_1_66](https://www.boost.org/doc/libs/1_66_0/more/getting_started/index.html)
 or higher to build the examples.

For examples, on windows, you can build boost library in these steps --

```
> cd *boost_source_folder*
> bootstrap.bat
> b2.exe stage variant=release runtime-link=static threading=multi
> b2.exe stage variant=debug runtime-link=static threading=multi
```

After have boost installed, modify path of boost library in the example's project file according to the real path.

### Example 1

This example shows converting a timer callback to promise object.

```cpp
#include <stdio.h>
#include <boost/asio.hpp>
#include "asio/timer.hpp"

using namespace promise;
using namespace boost::asio;

/* Convert callback to a promise (Defer) */
Defer myDelay(boost::asio::io_service &io, uint64_t time_ms) {
    return newPromise([&io, time_ms](Defer &d) {
        setTimeout(io, [d](bool cancelled) {
            if (cancelled)
                d.reject();
            else
                d.resolve();
        }, time_ms);
    });
}


Defer testTimer(io_service &io) {

    return myDelay(io, 3000).then([&] {
        printf("timer after 3000 ms!\n");
        return myDelay(io, 1000);
    }).then([&] {
        printf("timer after 1000 ms!\n");
        return myDelay(io, 2000);
    }).then([] {
        printf("timer after 2000 ms!\n");
    }).fail([] {
        printf("timer cancelled!\n");
    });
}

int main() {
    io_service io;

    Defer timer = testTimer(io);

    delay(io, 4500).then([=] {
        printf("clearTimeout\n");
        clearTimeout(timer);
    });

    io.run();
    return 0;
}
```

### Example 2

This example shows promise resolve/reject flows.

```cpp
#include <stdio.h>
#include <string>
#include "promise.hpp"

using namespace promise;

#define output_func_name() do{ printf("in function %s, line %d\n", __func__, __LINE__); } while(0)

void test1() {
    output_func_name();
}

int test2() {
    output_func_name();
    return 5;
}

void test3(int n) {
    output_func_name();
    printf("n = %d\n", n);
}

Defer run(Defer &next){

    return newPromise([](Defer d){
        output_func_name();
        d.resolve(3, 5, 6);
    }).then([](const int &a, int b, int c) {
        printf("%d %d %d\n", a, b, c);
        output_func_name();
    }).then([](){
        output_func_name();
    }).then([&next](){
        output_func_name();
        next = newPromise([](Defer d) {
            output_func_name();
        });
        //Will call next.resole() or next.reject() later
        return next;
    }).then([](int n, char c) {
        output_func_name();
        printf("n = %d, c = %c\n", (int)n, c);
    }).fail([](char n){
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](short n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](int &n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](const std::string &str) {
        output_func_name();
        printf("str = %s\n", str.c_str());
    }).fail([](uint64_t n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).then(test1)
    .then(test2)
    .then(test3)
    .always([]() {
        output_func_name();
    });
}

int main(int argc, char **argv) {
    Defer next;

    run(next);
    printf("======  after call run ======\n");

    next.resolve(123, 'a');
    //next.reject('c');
    //next.reject(std::string("xhchen"));
    //next.reject(45);

    return 0;
}
```

## Global functions

### Defer newPromise(FUNC func);
Creates a new Defer object with a user-defined function.
The user-defined functions, used as parameters by newPromise, must have a parameter Defer d. 
for example --

```cpp
return newPromise([](Defer d){
})
```

### Defer resolve(const RET_ARG... &ret_arg);
Returns a promise that is resolved with the given value.
for example --

```cpp
return resolve(3, '2');
```

### Defer reject(const RET_ARG... &ret_arg);
Returns a promise that is rejected with the given arguments.
for example --

```cpp
return reject("some_error");
```

### Defer all(const PROMISE_LIST &promise_list);
Wait until all promise objects in "promise_list" are resolved or one of which is rejected.
The "promise_list" can be any container that has Defer as element type.

> for (Defer &defer : promise_list) { ... }

for example --

```cpp
Defer d0 = newPromise([](Defer d){ /* ... */ });
Defer d1 = newPromise([](Defer d){ /* ... */ });
std::vector<Defer> promise_list = { d0, d1 };

all(promise_list).then([](){
    /* code here for all promise objects are resolved */
}).fail([](){
    /* code here for one of the promise objects is rejected */
});
```

### Defer race(const PROMISE_LIST &promise_list);
Rturns a promise that resolves or rejects as soon as one of
the promises in the iterable resolves or rejects, with the value
or reason from that promise.
The "promise_list" can be any container that has Defer as element type.

> for (Defer &defer : promise_list) { ... }

for example --

```cpp
Defer d0 = newPromise([](Defer d){ /* ... */ });
Defer d1 = newPromise([](Defer d){ /* ... */ });
std::vector<Defer> promise_list = { d0, d1 };

race(promise_list).then([](){
    /* code here for one of the promise objects is resolved */
}).fail([](){
    /* code here for one of the promise objects is rejected */
});
```

## Class Defer
Defer object is the promise object itself.

### Defer::resolve(const RET_ARG... &ret_arg);
Resolve the promise object with arguments, where you can put any number of ret_arg with any type.
(Please be noted that it is a method of Defer object, which is different from the global resolve function.)
for example --

```cpp
return newPromise([](Defer d){
    //d.resolve();
    //d.resolve(3, '2', std::string("abcd"));
    d.resolve(9567);
})
```

### Defer::reject(const RET_ARG... &ret_arg);
Reject the promise object with arguments, where you can put any number of ret_arg with any type.
(Please be noted that it is a method of Defer object, which is different from the global resolve function.)
for example --

```cpp
return newPromise([](Defer d){
    //d.reject();
    //d.reject(std::string("oh, no!"));
    d.reject(-1, std::string("oh, no!"))
})
```

### Defer::then(FUNC_ON_RESOLVED on_resolved, FUNC_ON_REJECTED on_rejected)
Return the chaining promise object, where on_resolved is the function to be called when 
previous promise object calls function resolve, on_rejected is the function to be called
when previous promise object calls function reject.
for example --

```cpp
return newPromise([](Defer d){
    d.resolve(9567, 'A');
}).then(
    /* function on_resolved */ [](int n, char ch){
        printf("%d %c\n", n, ch);   //will print 9567 here
    },
    /* function on_rejected */ [](){
         
    });
```

### Defer::then(FUNC_ON_RESOLVED on_resolved)
Return the chaining promise object, where on_resolved is the function to be called when 
previous promise object calls function resolve.
for example --

```cpp
return newPromise([](Defer d){
    d.resolve(9567);
}).then([](int n){
    printf("%d\n", n);  b //will print 9567 here
});
```

### Defer::fail(FUNC_ON_REJECTED on_rejected)
Return the chaining promise object, where on_rejected is the function to be called when
previous promise object calls function reject.

This function is usually named "catch" in most implements of Promise library. 
  https://www.promisejs.org/api/

In promise_cpp, function name "fail" is used instead of "catch", since "catch" is a keyword of c++.

for example --

```cpp
return newPromise([](Defer d){
    d.reject(-1, std::string("oh, no!"));
}).fail([](int err, string &str){
    printf("%d, %s\n", err, str.c_str());   //will print "-1, oh, no!" here
});
```

### Defer::finally(FUNC_ON_FINALLY on_finally)
Return the chaining promise object, where on_finally is the function to be called whenever
the previous promise object is be resolved or rejected.

The returned promise object will keeps the resolved/rejected state of current promise object.

for example --

```cpp
return newPromise([](Defer d){
    d.reject(std::string("oh, no!"));
}).finally([](){
    printf("in finally\n");   //will print "in finally" here
});
```

### Defer::always(FUNC_ON_ALWAYS on_always)
Return the chaining promise object, where on_always is the function to be called whenever
the previous promise object is be resolved or rejected.

The returned promise object will be in resolved state whenever current promise object is
resolved or rejected.

for example --

```cpp
return newPromise([](Defer d){
    d.reject(std::string("oh, no!"));
}).always([](){
    printf("in always\n");   //will print "in always" here
});
```

### about exceptions
To throw any object in the callback functions above, including on_resolved, on_rejected, on_always, 
will same as d.reject(the_throwed_object) and returns immediately.
for example --

```cpp
return newPromise([](Defer d){
    throw std::string("oh, no!");
}).fail([](string &str){
    printf("%s\n", str.c_str());   //will print "oh, no!" here
});
```
For the performance, we suggest to use function reject instead of throw.

### about the chaining parameter
Any type of parameter can be used when call resolve, reject or throw, except that the plain string or array.
To use plain string or array as chaining parameters, we may wrap it into an object.

```cpp
newPromise([](Defer d){
    // d.resolve("ok"); may cause a compiling error, use the following code instead.
    d.resolve(std::string("ok"));
})
```

### copy the promise object
To copy the promise object is allowed and effective, please do that when you need.

```cpp
Defer d = newPromise([](Defer d){});
Defer d1 = d;  //It's safe and effective
```

### handle uncaught exceptional or rejected parameters

The uncaught exceptional or rejected parameters are ignored by default. We can specify a handler function to do with these parameters --

```
handleUncaughtException([](Defer &d) {
    d.fail([](int n, int m) {
        //go here if the uncaught parameters match types "int n, int m".
    }).fail([](char c) {
        //go here if the uncaught parameters match type "char c".
    }).fail([]() {
        //go here for all other uncaught parameters.
    });
```





