#include <stdio.h>

#if 1
#include "promise.hpp"

#define PTI do{printf("%d, %s\n", __LINE__, __func__);} while(0)
using namespace promise;

void test1() {
    PTI;
}
int test2() {
    PTI;
    return 5;
}
void test3(int n) {
    PTI;
    printf("n = %d\n", n);
}

int main(int argc, char **argv) {
    Defer d1;

    newPromise([argc](Defer d){
        PTI;
		d->resolve();
	})->then([]() {
		Defer d2 = newPromise([](Defer d) {
			PTI;
			//d->resolve();
		});
		d2->resolve();
		return d2;
			//throw 333;
	})->then([](){
        PTI;
    })->then([](){
        PTI;
	})->fail([](){
        PTI;
	});
#if 0	
	->always([]() {
        PTI;
	});// ->then(test1)
		;//->then(test2)
    //->then(test3);
#endif
    printf("later=================\n");
    //(d1)->resolve();

    return 0;
}
#endif
