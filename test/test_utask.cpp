#include "../promise/utask.hpp"

#include <stdio.h>
#include <functional>
#define LINE do{printf("%d:%s\n", __LINE__, __func__);} while(0)

void task1(TaskPool *task_pool){
    LINE;
}

void task2(TaskPool *task_pool){
    LINE;
    task_pool->run_next_task();
    LINE;
    task_pool->exit_current_task();
    LINE;
}

void task_proxy(TaskPool *task_pool, void *arg){
    const std::function<void(TaskPool *)> func
        = *(const std::function<void(TaskPool *)> *)arg;
    (func)(task_pool);
}

inline void create_task(TaskPool *task_pool, const std::function<void(TaskPool *)> &func){
    task_pool->create_task(&task_proxy, (void *)&func);
}

int main(){
    TaskPool task_pool;

    LINE;

    create_task(&task_pool, task1);
    create_task(&task_pool, task2);    

    for(int i = 0; i < 10; ++i)
        create_task(&task_pool, [=](TaskPool *task_pool){
            unsigned int id = task_pool->current_task_id();
            for(int k = 0; k < 200; ++k){
                printf("task id = %d, i = %d, k = %d\n", id, i, k);
                task_pool->run_next_task();
            }
        });

    LINE;

    task_pool.loop();
    LINE;

    return 0;
}

