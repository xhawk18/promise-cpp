#pragma once
#ifndef INC_PM_LIST_HPP_
#define INC_PM_LIST_HPP_

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

namespace promise {

//List
struct pm_list {
    typedef pm_stack::itr_t itr_t;

    pm_list()
        : prev_(pm_stack::ptr_to_itr(reinterpret_cast<void *>(this)))
        , next_(pm_stack::ptr_to_itr(reinterpret_cast<void *>(this))) {
    }

    inline pm_list *prev() {
        return reinterpret_cast<pm_list *>(pm_stack::itr_to_ptr(prev_));
    }

    inline pm_list *next() {
        return reinterpret_cast<pm_list *>(pm_stack::itr_to_ptr(next_));
    }

    inline void prev(pm_list *other) {
        prev_ = pm_stack::ptr_to_itr(reinterpret_cast<void *>(other));
    }

    inline void next(pm_list *other) {
        next_ = pm_stack::ptr_to_itr(reinterpret_cast<void *>(other));
    }

    /* Connect or disconnect two lists. */
    static void toggleConnect(pm_list *list1, pm_list *list2) {
        pm_list *prev1 = list1->prev();
        pm_list *prev2 = list2->prev();
        prev1->next(list2);
        prev2->next(list1);
        list1->prev(prev2);
        list2->prev(prev1);
    }

    /* Connect two lists. */
    static void connect(pm_list *list1, pm_list *list2) {
        toggleConnect(list1, list2);
    }

    /* Disconnect tow lists. */
    static void disconnect(pm_list *list1, pm_list *list2) {
        toggleConnect(list1, list2);
    }

    /* Same as listConnect */
    void attach(pm_list *node) {
        connect(this, node);
    }

    /* Make node in detach mode */
    void detach () {
        disconnect(this, this->next());
    }

    /* Move node to list, after moving,
       node->next == this
       this->prev == node
     */
    void move(pm_list *node) {
#if 1
        node->prev()->next(node->next());
        node->next()->prev(node->prev());

        node->next(this);
        node->prev(this->prev());
        this->prev()->next(node);
        this->prev(node);
#else
        node->detach();
        attach(node);
#endif
    }

    /* Check if list is empty */
    int empty() {
        return (this->next() == this);
    }

private:
    itr_t prev_;
    itr_t next_;
};


}
#endif
