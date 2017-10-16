#pragma once
#ifndef INC_PM_UTASK_HPP_
#define INC_PM_UTASK_HPP_

/*
 * Promise API implemented by cpp as Javascript promise style 
 *
 * Copyright (c) 2016, xhawk18
 * at gmail.com
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifdef _WIN32
#   include <windows.h>
#else
#   define _XOPEN_SOURCE
#   include <ucontext.h>
#   include <unistd.h>
#endif
#include <stdio.h>
#include <cassert>
#include <list>
#include <vector>

static const size_t DEFAULT_STACK_SIZE = 1024*1024;

struct TaskPool;

struct Task{
#ifdef _WIN32
    LPVOID fiber_addr_;
    TaskPool *task_pool_;
    void (*func_)(TaskPool *, void *);
    void *arg_;
#else
    ucontext_t ucontext_;
    std::vector<char> stack_;
#endif
    unsigned int id_;
    Task(unsigned int id)
        : id_(id){
    }
};

struct TaskPool{
    unsigned int id_;
    std::list<Task> tasks_;
    Task *schedule_task_;

    TaskPool()
        : id_(0)
        , tasks_()
        , schedule_task_(create_task()){
#ifdef _WIN32
        schedule_task_->task_pool_ = this;
        schedule_task_->func_ = NULL;
        schedule_task_->arg_ = NULL;
        schedule_task_->fiber_addr_ = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
#endif
    }

    ~TaskPool(){
    }

    void run_next_task_ex(bool rewind_current){
        assert(tasks_.size() > 1);
        std::list<Task> current;
        current.splice(current.begin(), tasks_, tasks_.begin());

//#ifndef _WIN32
        Task *old_task = &current.front();
//#endif
        Task *new_task = &tasks_.front();

        if(rewind_current)
            tasks_.splice(tasks_.end(), current);

        //printf("rewind_current = %d, swap id %d => %d\n", (int)rewind_current, old_task->id_, new_task->id_);
#ifdef _WIN32
        //printf("%d, addr = %p => %p\n", (int)tasks_.size(), old_task->fiber_addr_, new_task->fiber_addr_);
        SwitchToFiber(new_task->fiber_addr_);
#else
        swapcontext(&old_task->ucontext_, &new_task->ucontext_);
#endif
    }

    void run_next_task(){
        run_next_task_ex(true);
    }

    void exit_current_task(){
        run_next_task_ex(false);
    }

#ifdef _WIN32
    static VOID CALLBACK task_main(PVOID arg){
        Task *task = reinterpret_cast<Task *>(arg);
        task->func_(task->task_pool_, task->arg_);
        task->task_pool_->exit_current_task();
    }
#else
    static void task_main(TaskPool *task_pool, void (*func)(TaskPool *, void *), void *arg){
        func(task_pool, arg);
        task_pool->exit_current_task();
    }
#endif

    Task *create_task(){
        auto pos = tasks_.begin();
        if(pos != tasks_.end())
            ++pos;
        return &*tasks_.emplace(pos, id_++);
    }

    Task *create_task(size_t stack_size, void (*func)(TaskPool *, void *), void *arg){
        Task *task = create_task();
#ifdef _WIN32
        task->task_pool_ = this;
        task->func_ = func;
        task->arg_ = arg;
        task->fiber_addr_ = CreateFiberEx(stack_size, stack_size, FIBER_FLAG_FLOAT_SWITCH, task_main, task);
#else
        getcontext(&task->ucontext_);
        task->stack_.resize(stack_size);
        task->ucontext_.uc_stack.ss_sp = task->stack_.data();
        task->ucontext_.uc_stack.ss_size = task->stack_.size();
        task->ucontext_.uc_link = &schedule_task_->ucontext_;
        makecontext(&task->ucontext_, (void (*)())task_main, 3, this, func, arg);
#endif
        run_next_task();
        return task;
    }
    
    Task *create_task(void (*func)(TaskPool *, void *), void *arg){
        return create_task(DEFAULT_STACK_SIZE, func, arg);
    }

    Task *current_task(){
        return &tasks_.front();
    }

    unsigned int current_task_id(){
        return current_task()->id_;
    }

    void loop(){
        while(tasks_.size() > 1)
            run_next_task();
    }
};

#endif

