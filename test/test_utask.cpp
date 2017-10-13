#include "../promise/utask.hpp"

#include <stdio.h>
#define LINE do{printf("%d:%s\n", __LINE__, __func__);} while(0)

void task1(TaskPool *task_pool, void *arg){
    LINE;
}

void task2(TaskPool *task_pool, void *arg){
    LINE;
    task_pool->run_next_task();
    LINE;
    task_pool->exit_current_task();
    LINE;
}

void task3(TaskPool *task_pool, void *arg){
    unsigned int id = task_pool->current_task_id();
    for(int k = 0; k < 100; ++k){
        printf("task id = %d, k = %d\n", id, k);
        task_pool->run_next_task();
    }
}


int main(){
    TaskPool task_pool;

    LINE;

    task_pool.create_task(&task1, NULL);
    task_pool.create_task(&task2, NULL);    

    for(int i = 0; i < 10; ++i)
        task_pool.create_task(&task3, NULL);    

    LINE;

    task_pool.loop();
    LINE;

    return 0;
}

