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


#define _XOPEN_SOURCE
#include <cassert>
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

#include <list>
#include <vector>

//static const size_t STACK_SIZE = 1024*1024;
static const size_t STACK_SIZE = 64*1024;

struct Task{
    ucontext_t ucontext_;
    unsigned int id_;
    std::vector<char> stack_;
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
    }

    ~TaskPool(){
    }

    void run_next_task_ex(bool rewind_current){
        assert(tasks_.size() > 1);
        std::list<Task> current;
        current.splice(current.begin(), tasks_, tasks_.begin());

        Task *old_task = &current.front();
        Task *new_task = &tasks_.front();

        if(rewind_current)
            tasks_.splice(tasks_.end(), current);

        //printf("swap id %d => %d\n", old_task->id_, new_task->id_);
        swapcontext(&old_task->ucontext_, &new_task->ucontext_);
    }

    void run_next_task(){
        run_next_task_ex(true);
    }

    void exit_current_task(){
        run_next_task_ex(false);
    }

    static void task_main(TaskPool *task_pool, void (*func)(TaskPool *, void *), void *arg){
        func(task_pool, arg);
        task_pool->exit_current_task();
    }


    Task *create_task(){
        tasks_.emplace_back(id_++);
        Task *task = &tasks_.back();
        return task;
    }

    Task *create_task(void (*func)(TaskPool *, void *), void *arg){
        Task *task = create_task();
        getcontext(&task->ucontext_);
        task->stack_.resize(STACK_SIZE);
        task->ucontext_.uc_stack.ss_sp = task->stack_.data();
        task->ucontext_.uc_stack.ss_size = task->stack_.size();
        task->ucontext_.uc_link = &schedule_task_->ucontext_;
        makecontext(&task->ucontext_, (void (*)())task_main, 3, this, func, arg);
        return task;
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

