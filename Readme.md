# C++ promise/A+ library in Javascript styles.

## Usage
   promise-cpp is header only and easy to use, just #include "promise.hpp" is enough.
   With promise-cpp, you can resolve or reject any type of data without writing complex template code.

## Example

### Example projects for linux

> Projects: build\codelite_linux\codelite.workspace

> Require: codelite, gcc 5 or higher

### Example projects for windows

> Projects: build\msvc\promise_cpp.sln

> Require: Visual studio 2015 or higher

or

> Projects: build\codelite_mingw\codelite.workspace

> Require: codelite, gcc 5 or higher


### Build tips

The example projects use boost::asio as io service. 
You need to install boost libary to build the examples.

On linux (ubuntu)

```
$ sudo apt-get install libboost-all-dev
```

On windows, you can build boost library manually.

```
> cd *boost_source_folder*
> bootstrap.bat
> b2.exe stage variant=release runtime-link=static threading=multi
> b2.exe stage variant=debug runtime-link=static threading=multi
```

Please also modify the boost library path in the example's project file.

### Example code

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





