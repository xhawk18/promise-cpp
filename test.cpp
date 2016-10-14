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
		printf("in function %s, line %d\n", __func__, __LINE__);
		d->resolve();
	}).then([&d1]() {
		Defer d2 = newPromise([](Defer d) {
			printf("in function %s, line %d\n", __func__, __LINE__);
			//d->resolve();
		});
		//d2.resolve();
        //throw 333;
		d2.reject(55);
		d1 = d2;
		return d2;
			//throw 333;
	}).then([](){
		printf("in function %s, line %d\n", __func__, __LINE__);
	}).then([](){
		printf("in function %s, line %d\n", __func__, __LINE__);
	}).fail([](int n){
		printf("in function %s, line %d   %d\n", __func__, __LINE__, n);
	}).always([]() {
		printf("in function %s, line %d\n", __func__, __LINE__);
	}).then(test1)
    .then(test2)
    .then(test3);

	printf("later=================\n");
    //(d1)->resolve();

    return 0;
}
#endif
