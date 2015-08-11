#include <stdio.h>
#include "promise.hpp"

#define PTI do{printf("%d, %s\n", __LINE__, __func__);} while(0)

int main(int argc, char **argv) {
    Defer d1;
    
    newPromise([argc, &d1](Defer d){
        PTI;
        d1 = d;
        d->resolve();
        //d->reject();
    })->then([](){
        PTI;
        Defer d2 = newPromise([](Defer d){
            PTI;
            d->resolve();
        });
        d2->resolve();
        return d2;
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
