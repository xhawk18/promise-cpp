#include <stdio.h>

#if 1
#include "promise.hpp"

using namespace promise;

void test1() {
	printf("in function %s, line %d\n", __func__, __LINE__);
}
int test2() {
	printf("in function %s, line %d\n", __func__, __LINE__);
    return 5;
}
void test3(int n) {
	printf("in function %s, line %d\n", __func__, __LINE__);
    printf("n = %d\n", n);
}

int main(int argc, char **argv) {
    Defer d1;

    newPromise([argc](Defer d){
		printf("in function %s, line %d\n", __func__, __LINE__);
		d->resolve();
	}).then([]() {
		printf("in function %s, line %d\n", __func__, __LINE__);
	}).then([](){
		printf("in function %s, line %d\n", __func__, __LINE__);
	}).then([&d1](){
		printf("in function %s, line %d\n", __func__, __LINE__);
		d1 = newPromise([](Defer d) {
			printf("in function %s, line %d\n", __func__, __LINE__);
			//d->resolve();
		});
		return d1;
	}).fail([](int n){
		printf("in function %s, line %d, failed with value %d\n", __func__, __LINE__, n);
	}).then(test1)
    .then(test2)
    .then(test3)
	.always([]() {
		printf("in function %s, line %d\n", __func__, __LINE__);
	});

	printf("call later=================\n");
    //d1.resolve();
	d1.reject(123);

    return 0;
}
#endif
