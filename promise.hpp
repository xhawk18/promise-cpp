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
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_

#include <exception>
#include <memory>
#include <tuple>
#include <typeinfo>
#include <typeindex>
#include <type_traits>

namespace promise {


template<class P, class M>
inline size_t pm_offsetof(const M P::*member) {
    return (size_t)&reinterpret_cast<char const&>((reinterpret_cast<P*>(0)->*member));
}

template<class P, class M>
inline P* pm_container_of(M* ptr, const M P::*member) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) - pm_offsetof(member));
}

//List
struct list_t {
    struct list_t *next_;
    struct list_t *prev_;
};

inline list_t *listGetPrev (list_t *list) {
    return list->prev_;
}

inline list_t *listGetNext (list_t *list) {
    return list->next_;
}

inline void listSetPrev (list_t *list, list_t *other) {
    list->prev_ = other;
}

inline void listSetNext (list_t *list, list_t *other) {
    list->next_ = other;
}

/* Initilization a list */
inline void listInit(list_t *list) {
    listSetPrev(list, list);
    listSetNext(list, list);
}

/* Connect or disconnect two lists. */
inline void listToggleConnect(list_t *list1, list_t *list2) {
    list_t *prev1 = listGetPrev(list1);
    list_t *prev2 = listGetPrev(list2);
    listSetNext(prev1, list2);
    listSetNext(prev2, list1);
    listSetPrev(list1, prev2);
    listSetPrev(list2, prev1);
}

/* Connect two lists. */
inline void listConnect (list_t *list1, list_t *list2) {
    listToggleConnect (list1, list2);
}

/* Disconnect tow lists. */
inline void listDisconnect (list_t *list1, list_t *list2) {
    listToggleConnect (list1, list2);
}

/* Same as listConnect */
inline void listAttach (list_t *node1, list_t *node2) {
    listConnect (node1, node2);
}

/* Make node in detach mode */
inline void listDetach (list_t *node) {
    listDisconnect (node, listGetNext(node));
}

/* Move node to list, after moving,
   node->next == list
   list->prev == node
 */
inline void listMove(list_t *list, list_t *node) {
#if 1
    node->prev_->next_ = node->next_;
    node->next_->prev_ = node->prev_;

    node->next_ = list;
    node->prev_ = list->prev_;
    list->prev_->next_ = node;
    list->prev_ = node;
#else
    listDetach(node);
    listAttach(list, node);
#endif
}

/* Check if list is empty */
inline int listIsEmpty (list_t *list) {
    return (listGetNext(list) == list);
}


//allocator
template <size_t SIZE>
struct memory_pool_buf {
    struct buf_t {
        buf_t(){}
        void *buf[(SIZE + sizeof(void *) - 1) / sizeof(void *)];
    };

    memory_pool_buf() {
        listInit(&list_);
    }
    buf_t buf_;
    list_t list_;
};

template <size_t SIZE>
struct memory_pool {
    list_t used_;
    list_t free_;
    memory_pool(){
        listInit(&used_);
        listInit(&free_);
    }
};

template <size_t SIZE>
struct size_allocator {
    static inline memory_pool<SIZE> *get_memory_pool() {
        static memory_pool<SIZE> *pool_ = nullptr;
        if(pool_ == nullptr)
            pool_ = new memory_pool<SIZE>();
        return pool_;
    }
};

template <typename T>
struct allocator {
    enum allocator_type_t {
        kNew,
        kMalloc,
        kListPool
    };

    static const allocator_type_t allocator_type = kListPool;

    static inline void *obtain(size_t size) {
        if(allocator_type == kNew)
            return new typename memory_pool_buf<sizeof(T)>::buf_t;
        else if(allocator_type == kMalloc)
            return malloc(size);
        else if(allocator_type == kListPool){
            memory_pool<sizeof(T)> *pool = size_allocator<sizeof(T)>::get_memory_pool();
            if (listIsEmpty(&pool->free_)) {
                memory_pool_buf<sizeof(T)> *pool_buf = new memory_pool_buf<sizeof(T)>;
                listAttach(&pool->used_, &pool_buf->list_);
                return (void *)&pool_buf->buf_;
            }
            else {
                list_t *node = listGetNext(&pool->free_);
                listMove(&pool->used_, node);
                memory_pool_buf<sizeof(T)> *pool_buf = pm_container_of<memory_pool_buf<sizeof(T)>, list_t>
                    (node, &memory_pool_buf<sizeof(T)>::list_);
                return (void *)&pool_buf->buf_;
            }
        }
        else return NULL;
    }
    
    static inline void release(void *ptr) {
        if(allocator_type == kNew){
            delete reinterpret_cast<typename memory_pool_buf<sizeof(T)>::buf_t *>(ptr);
            return;
        }
        else if(allocator_type == kMalloc){
            free(ptr);
            return;
        }
        else if(allocator_type == kListPool){
            memory_pool<sizeof(T)> *pool = size_allocator<sizeof(T)>::get_memory_pool();
            memory_pool_buf<sizeof(T)> *pool_buf = pm_container_of<memory_pool_buf<sizeof(T)>, typename memory_pool_buf<sizeof(T)>::buf_t>
                ((typename memory_pool_buf<sizeof(T)>::buf_t *)ptr, &memory_pool_buf<sizeof(T)>::buf_);
            listMove(&pool->free_, &pool_buf->list_);
        }
    }
};


// Any library
// See http://www.boost.org/libs/any for Documentation.
// what:  variant type any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

template<typename T>
struct remove_rcv {
    typedef typename std::remove_reference<T>::type r_type;
    typedef typename std::remove_cv<r_type>::type type;
};

template<typename T>
struct remove_rcv<const T *> {
    typedef typename remove_rcv<T *>::type type;
};

template<typename T>
struct remove_rcv<const T &> {
    typedef typename remove_rcv<T &>::type type;
};

template<typename T>
struct void_ptr_type {
    static void *cast(const T &t) {
        return reinterpret_cast<void *>(const_cast<char *>(&reinterpret_cast<const char &>(t)));
    }
};

template<typename T>
struct void_ptr_type<const T *> {
    typedef const T* PT;
    static void *cast(const PT &t) {
        return (void *)(&t);
    }
};

template<typename T>
void *void_ptr_cast(const T &t) {
    return void_ptr_type<T>::cast(t);
}

template<typename TUPLE, std::size_t SIZE>
struct offset_tuple_impl {
    template<size_t I_SIZE, std::size_t... I>
    struct offset_array {
        void *offsets_[I_SIZE];

        offset_array(const TUPLE *tuple)
            : offsets_{ void_ptr_cast(std::get<I>(*tuple))... } {
        }
    };

    template<std::size_t... I>
    static offset_array<SIZE, I...> get_array(const TUPLE *tuple, const std::index_sequence<I...> &) {
        return offset_array<SIZE, I...>(tuple);
    }

    decltype(get_array(nullptr, std::make_index_sequence<SIZE>())) value_;

    offset_tuple_impl(const TUPLE *tuple)
        : value_(get_array(tuple, std::make_index_sequence<SIZE>())) {
    }

    void *tuple_offset(size_t i) const {
        return value_.offsets_[i];
    }
};

template<typename TUPLE>
struct offset_tuple_impl<TUPLE, 0> {
    offset_tuple_impl(const TUPLE *tuple) {
    }
    void *tuple_offset(size_t i) const {
        return nullptr;
    }
};

template<typename NOT_TUPLE>
struct offset_tuple
    : public offset_tuple_impl<NOT_TUPLE, 0> {
    offset_tuple(const NOT_TUPLE *tuple)
        : offset_tuple_impl<NOT_TUPLE, 0>(tuple) {
    }
};

template<typename ...T>
struct offset_tuple<std::tuple<T...>>
    : public offset_tuple_impl<std::tuple<T...>, std::tuple_size<std::tuple<T...>>::value> {
    offset_tuple(const std::tuple<T...> *tuple)
        : offset_tuple_impl<std::tuple<T...>, std::tuple_size<std::tuple<T...>>::value>(tuple) {
    }
};

template<typename TUPLE, std::size_t SIZE>
struct type_tuple_impl {
    template<size_t I_SIZE, std::size_t... I>
    struct type_index_array {
        std::type_index types_[I_SIZE];
        std::type_index types_rcv_[I_SIZE];

        type_index_array()
            : types_{ std::type_index(typeid(typename std::tuple_element<I, TUPLE>::type))... }
            , types_rcv_{ std::type_index(typeid(typename remove_rcv<typename std::tuple_element<I, TUPLE>::type>::type))... } {
        }
    };

    template<std::size_t... I>
    static type_index_array<SIZE, I...> get_array(const std::index_sequence<I...> &) {
        return type_index_array<SIZE, I...>();
    }

    decltype(get_array(std::make_index_sequence<SIZE>())) value_;

    type_tuple_impl()
        : value_(get_array(std::make_index_sequence<SIZE>())) {
    }

    const std::type_index tuple_type(size_t i) const {
        return value_.types_[i];
    }

    const std::type_index tuple_rcv_type(size_t i) const {
        return value_.types_rcv_[i];
    }
};

template<typename TUPLE>
struct type_tuple_impl<TUPLE, 0> {
    static const std::size_t size_ = 0;
    const std::type_index tuple_type(size_t i) const {
        return std::type_index(typeid(void));
    }
    const std::type_index tuple_rcv_type(size_t i) const {
        return std::type_index(typeid(void));
    }
};

template<typename NOT_TUPLE>
struct type_tuple :
    public type_tuple_impl<void, 0> {
    static const std::size_t size_ = 0;
};

template<typename ...T>
struct type_tuple<std::tuple<T...>> :
    public type_tuple_impl<std::tuple<T...>, std::tuple_size<std::tuple<T...>>::value> {
    static const std::size_t size_ = std::tuple_size<std::tuple<T...>>::value;
};

template<typename T>
struct tuple_remove_reference {
    typedef typename std::remove_reference<T>::type r_type;
    typedef typename std::remove_cv<r_type>::type type;
};

template<typename T, std::size_t SIZE>
struct tuple_remove_reference<T[SIZE]> {
    typedef typename tuple_remove_reference<const T *>::type type;
};

template<typename TUPLE>
struct remove_reference_tuple {
    static const std::size_t size_ = std::tuple_size<TUPLE>::value;

    template<size_t SIZE, std::size_t... I>
    struct converted {
        typedef std::tuple<typename tuple_remove_reference<typename std::tuple_element<I, TUPLE>::type>::type...> type;
    };

    template<std::size_t... I>
    static converted<size_, I...> get_type(const std::index_sequence<I...> &) {
        return converted<size_, I...>();
    }

    typedef decltype(get_type(std::make_index_sequence<size_>())) converted_type;
    typedef typename converted_type::type type;
};

template<typename FUNC>
struct func_traits_impl {
    typedef decltype(&FUNC::operator()) func_type;
    typedef typename func_traits_impl<func_type>::ret_type ret_type;
    typedef typename func_traits_impl<func_type>::arg_type arg_type;
};

template<typename RET, class T, typename ...ARG>
struct func_traits_impl< RET(T::*)(ARG...) const > {
    typedef RET ret_type;
    typedef std::tuple<ARG...> arg_type;
};

template<typename RET, typename ...ARG>
struct func_traits_impl< RET(*)(ARG...) > {
    typedef RET ret_type;
    typedef std::tuple<ARG...> arg_type;
};

template<typename RET, typename ...ARG>
struct func_traits_impl< RET(ARG...) > {
    typedef RET ret_type;
    typedef std::tuple<ARG...> arg_type;
};

template<typename FUNC>
struct func_traits {
    typedef typename func_traits_impl<FUNC>::ret_type ret_type;
    typedef typename remove_reference_tuple<typename func_traits_impl<FUNC>::arg_type>::type arg_type;
};


class any {
public: // structors
    any()
        : content(0) {
    }

    template<typename ValueType>
    any(const ValueType & value)
        : content(new holder<ValueType>(value)) {
    }

    any(const any & other)
        : content(other.content ? other.content->clone() : 0) {
    }

    ~any() {
        delete content;
    }

public: // modifiers

    any & swap(any & rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    template<typename ValueType>
    any & operator=(const ValueType & rhs) {
        any(rhs).swap(*this);
        return *this;
    }

    any & operator=(const any & rhs) {
        any(rhs).swap(*this);
        return *this;
    }

public: // queries
    bool empty() const {
        return !content;
    }
    
    void clear() {
        any().swap(*this);
    }

    const std::type_info & type() const {
        return content ? content->type() : typeid(void);
    }

    const std::size_t tuple_size() const {
        return content ? content->tuple_size() : 0;
    }

    const std::type_index tuple_type(size_t i) const {
        return content ? content->tuple_type(i) : std::type_index(typeid(void));
    }

    void *tuple_element(size_t i) const {
        return content ? content->tuple_element(i) : nullptr;
    }

public: // types (public so any_cast can be non-friend)
    class placeholder {
    public: // structors
        virtual ~placeholder() {
        }

    public: // queries
        virtual const std::type_info & type() const = 0;
        virtual const std::size_t tuple_size() const = 0;
        virtual const std::type_index tuple_type(size_t i) const = 0;
        virtual void *tuple_element(size_t i) const = 0;

        virtual placeholder * clone() const = 0;
    };

    template<typename ValueType>
    class holder : public placeholder {
    public: // structors
#if 1
        void* operator new(size_t size) {
            return allocator<holder>::obtain(size);
        }

        void operator delete(void *ptr) {
            allocator<holder>::release(ptr);
        }
#endif

        holder(const ValueType & value)
            : held(value)
            , type_tuple_()
            , offset_tuple_(&held) {
        }

    public: // queries
        virtual const std::type_info & type() const {
            return typeid(ValueType);
        }

        virtual const std::size_t tuple_size() const {
            return type_tuple_.size_;
        }

        virtual const std::type_index tuple_type(size_t i) const {
            return type_tuple_.tuple_type(i);
        }

        virtual void *tuple_element(size_t i) const {
            return offset_tuple_.tuple_offset(i);
        }

        virtual placeholder * clone() const {
            return new holder(held);
        }
    public: // representation
        ValueType held;
        type_tuple<ValueType> type_tuple_;
        offset_tuple<ValueType> offset_tuple_;
    private: // intentionally left unimplemented
        holder & operator=(const holder &);
    };

public: // representation (public so any_cast can be non-friend)
    placeholder * content;
};

class bad_any_cast : public std::bad_cast {
public:
    std::type_index from_;
    std::type_index to_;
    bad_any_cast(const std::type_index &from, const std::type_index &to)
        : from_(from)
        , to_(to) {
    }
    virtual const char * what() const throw() {
        return "bad_any_cast: "
            "failed conversion using any_cast";
    }
};

template<typename ValueType>
ValueType * any_cast(any *operand) {
    typedef typename any::template holder<ValueType> holder_t;
    return operand &&
        operand->type() == typeid(ValueType)
        ? &static_cast<holder_t *>(operand->content)->held
        : 0;
}

template<typename ValueType>
inline const ValueType * any_cast(const any *operand) {
    return any_cast<ValueType>(const_cast<any *>(operand));
}

template<typename ValueType>
ValueType any_cast(any & operand) {
    typedef typename std::remove_reference<ValueType>::type nonref;

    nonref * result = any_cast<nonref>(&operand);
    if (!result)
        throw bad_any_cast(std::type_index(operand.type()), std::type_index(typeid(ValueType)));
    return *result;
}

template<typename ValueType>
inline ValueType any_cast(const any &operand) {
    typedef typename std::remove_reference<ValueType>::type nonref;
    return any_cast<const nonref &>(const_cast<any &>(operand));
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



template<typename RET, typename FUNC, std::size_t ...I>
struct call_tuple_t {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    typedef typename remove_reference_tuple<std::tuple<RET>>::type ret_type;
    
    static ret_type call(const FUNC &func, any &arg) {
        func_arg_type new_arg(*reinterpret_cast<typename std::tuple_element<I, func_arg_type>::type *>(arg.tuple_element(I))...);
        arg.clear();
        return ret_type(func(std::get<I>(new_arg)...));
    }
};

template<typename FUNC, std::size_t ...I>
struct call_tuple_t<void, FUNC, I...> {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;

    static std::tuple<> call(const FUNC &func, any &arg) {
        func_arg_type new_arg(*reinterpret_cast<typename std::tuple_element<I, func_arg_type>::type *>(arg.tuple_element(I))...);
        arg.clear();
        func(std::get<I>(new_arg)...);
        return std::tuple<>();
    }
};

template<typename FUNC, std::size_t ...I>
inline auto call_tuple_as_argument(const FUNC &func, any &arg, const std::index_sequence<I...> &) {
    typedef typename func_traits<FUNC>::ret_type ret_type;

    return call_tuple_t<ret_type, FUNC, I...>::call(func, arg);
}

template<typename FUNC>
inline auto call_func(const FUNC &func, any &arg) {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    type_tuple<func_arg_type> tuple_func;

    if (arg.tuple_size() < tuple_func.size_)
        throw bad_any_cast(std::type_index(arg.type()), std::type_index(typeid(func_arg_type)));

    for (size_t i = tuple_func.size_; i-- != 0; ) {
        if (arg.tuple_type(i) != tuple_func.tuple_type(i)
            && arg.tuple_type(i) != tuple_func.tuple_rcv_type(i)) {
            //printf("== %s ==> %s\n", arg.tuple_type(i).name(), tuple_func.tuple_type(i).name());
            throw bad_any_cast(arg.tuple_type(i), tuple_func.tuple_type(i));
        }
    }

    return call_tuple_as_argument(func, arg, std::make_index_sequence<tuple_func.size_>());
}


struct Promise;

template< class T >
class shared_ptr {
    typedef shared_ptr<Promise> Defer;
public:
    virtual ~shared_ptr() {
        dec_ref();
    }

    explicit shared_ptr(T *object)
        : object_(object) {
        add_ref();
    }
    
    explicit shared_ptr()
        : object_(nullptr) {
    }

    shared_ptr(shared_ptr const &ptr)
        : object_(ptr.object_) {
        add_ref();
    }

    shared_ptr &operator=(shared_ptr const &ptr) {
        shared_ptr(ptr).swap(*this);
        return *this;
    }

    bool operator==(shared_ptr const &ptr) const {
        return object_ == ptr.object_;
    }

    bool operator!=(shared_ptr const &ptr) const {
        return !( *this == ptr );
    }

    bool operator==(T const *ptr) const {
        return object_ == ptr;
    }

    bool operator!=(T const *ptr) const {
        return !( *this == ptr );
    }

    inline T *operator->() const {
        return object_;
    }
    
    inline T *obtain_rawptr(){
        add_ref();
        return object_;
    }
    
    inline void release_rawptr(){
        dec_ref();
    }

    Defer find_pending() const {
        return object_->find_pending();
    }
    
    void clear() {
        shared_ptr().swap(*this);
    }

    template <typename ...RET_ARG>
    void resolve(const RET_ARG &... ret_arg) const {
        object_->template resolve<RET_ARG...>(ret_arg...);
    }

    template <typename ...RET_ARG>
    void reject(const RET_ARG &... ret_arg) const {
        object_->template reject<RET_ARG...>(ret_arg...);
    }

    template <typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
    Defer then(FUNC_ON_RESOLVED on_resolved, FUNC_ON_REJECTED on_rejected) const {
        return object_->template then<FUNC_ON_RESOLVED, FUNC_ON_REJECTED>(on_resolved, on_rejected);
    }

    template <typename FUNC_ON_RESOLVED>
    Defer then(FUNC_ON_RESOLVED on_resolved) const {
        return object_->template then<FUNC_ON_RESOLVED>(on_resolved);
    }

    template <typename FUNC_ON_REJECTED>
    Defer fail(FUNC_ON_REJECTED on_rejected) const {
        return object_->template fail<FUNC_ON_REJECTED>(on_rejected);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) const {
        return object_->template always<FUNC_ON_ALWAYS>(on_always);
    }   
private:
    void add_ref() {
        if (object_ != nullptr) {
            //printf("++ %p %d -> %d\n", object_, object_->ref_count_, object_->ref_count_ + 1);
            ++object_->ref_count_;
        }
    }
    
    void dec_ref() {
        if(object_ != nullptr) {
            //printf("-- %p %d -> %d\n", object_, object_->ref_count_, object_->ref_count_ - 1);
            --object_->ref_count_;
            if(object_->ref_count_ == 0)
                delete object_;
        }
    }

    inline void swap(shared_ptr &ptr) {
        std::swap(object_, ptr.object_);
    }

    T *object_;
};

typedef shared_ptr<Promise> Defer;

typedef void(*FnSimple)();

template <typename RET, typename FUNC>
struct ResolveChecker;
template <typename RET, typename FUNC>
struct RejectChecker;

template <typename Promise, typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
struct PromiseEx 
    : public Promise {
    typedef typename func_traits<FUNC_ON_RESOLVED>::ret_type resolve_ret_type;
    typedef typename func_traits<FUNC_ON_REJECTED>::ret_type reject_ret_type;

    struct {
        void *buf[(sizeof(FUNC_ON_RESOLVED) + sizeof(void *) - 1)/ sizeof(void *)];
    } func_buf0_t;
    struct {
        void *buf[(sizeof(FUNC_ON_REJECTED) + sizeof(void *) - 1) / sizeof(void *)];
    } func_buf1_t;

    PromiseEx(const FUNC_ON_RESOLVED &on_resolved, const FUNC_ON_REJECTED &on_rejected)
        : Promise(reinterpret_cast<void *>(new(&func_buf0_t) FUNC_ON_RESOLVED(on_resolved)),
            reinterpret_cast<void *>(new(&func_buf1_t) FUNC_ON_REJECTED(on_rejected))) {
    }

    virtual ~PromiseEx() {
        clear_func();
    }
    
    void clear_func() {
        if(Promise::on_resolved_ != nullptr) {
            reinterpret_cast<FUNC_ON_RESOLVED *>(Promise::on_resolved_)->~FUNC_ON_RESOLVED();
            Promise::on_resolved_ = nullptr;
        }
        if(Promise::on_rejected_ != nullptr) {
            reinterpret_cast<FUNC_ON_REJECTED *>(Promise::on_rejected_)->~FUNC_ON_REJECTED();
            Promise::on_rejected_ = nullptr;
        }
    }
    
    virtual Defer call_resolve(Defer &self, Promise *caller) {
        const FUNC_ON_RESOLVED &on_resolved = *reinterpret_cast<FUNC_ON_RESOLVED *>(Promise::on_resolved_);
        Defer d = ResolveChecker<resolve_ret_type, FUNC_ON_RESOLVED>::call(on_resolved, self, caller);
        caller->any_.clear();
        clear_func();
        return d;
    }
    virtual Defer call_reject(Defer &self, Promise *caller) {
        const FUNC_ON_REJECTED &on_rejected = *reinterpret_cast<FUNC_ON_REJECTED *>(Promise::on_rejected_);
        Defer d = RejectChecker<reject_ret_type, FUNC_ON_REJECTED>::call(on_rejected, self, caller);
        caller->any_.clear();
        clear_func();
        return d;
    }

    void* operator new(size_t size) {
        return allocator<PromiseEx>::obtain(size);
    }
    
    void operator delete(void *ptr) {
        allocator<PromiseEx>::release(ptr);
    }
};

struct Promise {
    int ref_count_;
    Promise *prev_;
    Defer next_;
    any any_;
    void *on_resolved_;
    void *on_rejected_;
    
    enum status_t {
        kInit,
        kResolved,
        kRejected,
        kFinished
    };
    status_t status_;
    
    Promise(const Promise &) = delete;
    explicit Promise(void *on_resolved, void *on_rejected)
        : ref_count_(0)
        , prev_(nullptr)
        , next_(nullptr)
        , on_resolved_(on_resolved)
        , on_rejected_(on_rejected)
        , status_(kInit) {
    }
    
    virtual ~Promise() {
        if (next_.operator->()) {
            next_->prev_ = nullptr;
        }
    }
    
    template <typename RET_ARG>
    void prepare_resolve(const RET_ARG &ret_arg) {
        if (status_ != kInit) return;
        status_ = kResolved;
        any_ = ret_arg;
    }

    template <typename ...RET_ARG>
    void resolve(const RET_ARG &... ret_arg) {
        typedef typename remove_reference_tuple<std::tuple<RET_ARG...>>::type arg_type;
        prepare_resolve(arg_type(ret_arg...));
        if(status_ == kResolved)
            call_next();
    }

    template <typename RET_ARG>
    void prepare_reject(const RET_ARG &ret_arg) {
        if (status_ != kInit) return;
        status_ = kRejected;
        any_ = ret_arg;
    }

    template <typename ...RET_ARG>
    void reject(const RET_ARG &...ret_arg) {
        typedef typename remove_reference_tuple<std::tuple<RET_ARG...>>::type arg_type;
        prepare_reject(arg_type(ret_arg...));
        if(status_ == kRejected)
            call_next();
    }

    virtual Defer call_resolve(Defer &self, Promise *caller) = 0;
    virtual Defer call_reject(Defer &self, Promise *caller) = 0;

    template <typename FUNC>
    void run(FUNC func, Defer d) {
        try {
            func(d);
        } catch(const bad_any_cast &ex) {
            d->reject(ex);
        } catch(...) {
            d->reject(std::current_exception());
        }
    }
    
    Defer call_next() {
        if(status_ == kResolved) {
            if(next_.operator->()){
                status_ = kFinished;
                Defer d = next_->call_resolve(next_, this);
                if(d.operator->())
                    d->call_next();
                return d;
            }
        }
        else if(status_ == kRejected ) {
            if(next_.operator->()){
                status_ = kFinished;
                Defer d =  next_->call_reject(next_, this);
                if (d.operator->())
                    d->call_next();
                return d;
            }
        }

        return next_;
    }

    template <typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
    Defer then(FUNC_ON_RESOLVED on_resolved, FUNC_ON_REJECTED on_rejected) {
        Defer promise(new PromiseEx<Promise, FUNC_ON_RESOLVED, FUNC_ON_REJECTED>(on_resolved, on_rejected));
        next_ = promise;
        promise->prev_ = this;
        return call_next();
    }

    template <typename FUNC_ON_RESOLVED>
    Defer then(FUNC_ON_RESOLVED on_resolved) {
        return then<FUNC_ON_RESOLVED, FnSimple>(on_resolved, nullptr);
    }

    template <typename FUNC_ON_REJECTED>
    Defer fail(FUNC_ON_REJECTED on_rejected) {
        return then<FnSimple, FUNC_ON_REJECTED>(nullptr, on_rejected);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) {
        return then<FUNC_ON_ALWAYS, FUNC_ON_ALWAYS>(on_always, on_always);
    }

    Defer find_pending() {
        if (status_ == kInit) {
            Promise *p = this;
            Promise *prev = p->prev_;
            while (prev != nullptr) {
                if (prev->status_ != kInit)
                    return prev->next_;
                p = prev;
                prev = p->prev_;
            }
            return Defer(p);
        }
        else {
            Promise *p = this;
            Promise *next = p->next_.operator->();
            while (next != nullptr) {
                if (next->status_ == kInit)
                    return p->next_;
                p = next;
                next = p->next_.operator->();
            }
            return Defer();
        }
    }
};

inline void joinDeferObject(Defer &self, Defer &next){
    if(self->next_.operator->())
        self->next_->prev_ = next.operator->();
    next->next_ = self->next_;
    self->next_ = next;
    next->prev_ = self.operator->();
}

template <typename RET, typename FUNC>
struct ResolveChecker {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            self->prepare_resolve(call_func(func, caller->any_));
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename FUNC>
struct ResolveChecker<Defer, FUNC> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            Defer ret = std::get<0>(call_func(func, caller->any_));
            joinDeferObject(self, ret);
            return ret;
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};


template <typename RET>
struct ResolveChecker<RET, FnSimple> {
    static Defer call(const FnSimple &func, Defer &self, Promise *caller) {
        try {
            if (func != nullptr)
                self->prepare_resolve(call_func(func, caller->any_));
            else {
                self->prepare_resolve(caller->any_);
            }
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template<std::size_t ARG_SIZE, typename FUNC>
struct ExCheck {
    static void call(const FUNC &func, Defer &self, Promise *caller) {
        std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
        throw eptr;
    }
};

template <typename FUNC>
struct ExCheck<0, FUNC> {
    static auto call(const FUNC &func, Defer &self, Promise *caller) {
        any arg = std::tuple<>();
        caller->any_.clear();
        return call_func(func, arg);
    }
};

template <typename FUNC>
struct ExCheck<1, FUNC> {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static auto call(const FUNC &func, Defer &self, Promise *caller) {
        std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
        try {
            std::rethrow_exception(eptr);
        }
        catch (const typename std::tuple_element<0, arg_type>::type &ret_arg) {
            any arg = arg_type(ret_arg);
            caller->any_.clear();
            return call_func(func, arg);
        }

        /* Will never run to here, just make the compile satisfied! */
        any arg;
        return call_func(func, arg);
    }
};

template <typename RET, typename FUNC>
struct RejectChecker {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                self->prepare_resolve(ExCheck<std::tuple_size<arg_type>::value, FUNC>::call(func, self, caller));
            }
            else {
                self->prepare_resolve(call_func(func, caller->any_));
            }
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename FUNC>
struct RejectChecker<Defer, FUNC> {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                Defer ret = std::get<0>(ExCheck<std::tuple_size<arg_type>::value, FUNC>::call(func, self, caller));
                joinDeferObject(self, ret);
                return ret;
            }
            else{
                Defer ret = std::get<0>(call_func(func, caller->any_));
                joinDeferObject(self, ret);
                return ret;
            }
        }
        catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
        }
        catch(...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename RET>
struct RejectChecker<RET, FnSimple> {
    static Defer call(const FnSimple &func, Defer &self, Promise *caller) {
        try {
            if (func != nullptr) {
                self->prepare_resolve(call_func(func, caller->any_));
                return self;
            }
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            return self;
        } catch(...) {
            self->prepare_reject(std::current_exception());
            return self;
        }

        self->prepare_reject(caller->any_);
        return self;
    }
};


template <typename FUNC>
Defer newPromise(FUNC func) {
    /* Here func in PromiseEx will never be called.
       To save func in PromiseEx is just to keep reference of the object */
    Defer promise(new PromiseEx<Promise, FnSimple, FnSimple>(nullptr, nullptr));
    promise->run(func, promise);
    return promise;
}


}
#endif
