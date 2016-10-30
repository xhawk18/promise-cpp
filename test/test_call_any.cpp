#include <tuple>
#include <typeinfo>
#include <typeindex>
#include <type_traits>

#if 0
template<typename T>
struct un_reference {
    typedef T type;
};
template<typename T>
struct un_reference<T&> {
    typedef T type;
};
template<typename T>
struct un_reference<const T&> {
    typedef T type;
};
template<typename T>
struct un_reference<T&&> {
    typedef typename un_reference<T&>::type type;
};
template<typename T>
struct un_reference<const T&&> {
    typedef typename un_reference<T&>::type type;
};


template<typename FUNC>
struct get_arg_type {
    typedef decltype(&FUNC::operator()) func_type;
    static const size_t arg_size = get_arg_type<func_type>::arg_size;
    typedef typename get_arg_type<func_type>::tuple_type tuple_type;
};

template<typename RET, typename T, typename ...ARG>
struct get_arg_type< RET(T::*)(ARG...) const > {
    static const size_t arg_size = std::tuple_size<std::tuple<ARG...>>::value;
    typedef std::tuple<typename un_reference<ARG>::type...> tuple_type;
};

template<typename RET, typename ...ARG>
struct get_arg_type< RET(*)(ARG...) > {
    static const size_t arg_size = std::tuple_size<std::tuple<ARG...>>::value;
    typedef std::tuple<typename un_reference<ARG>::type...> tuple_type;
};

template<typename RET, typename ...ARG>
struct get_arg_type< RET(ARG...) > {
    static const size_t arg_size = std::tuple_size<std::tuple<ARG...>>::value;
    typedef std::tuple<typename un_reference<ARG>::type...> tuple_type;
};

template<std::size_t s0, std::size_t s1>
struct get_min {
    static const std::size_t value = (s0 < s1 ? s0 : s1);
};

template<typename F, typename T, std::size_t... I>
auto apply_impl(F f, const T& t, std::index_sequence<I...>) -> decltype(f(std::get<I>(t)...))
{
    return f(std::get<I>(t)...);
}

template<typename F, typename T>
auto apply(F f, const T& t) {
    //get_arg_type<F>::tuple_type tt 
    return apply_impl(f, t, std::make_index_sequence<
        get_min<std::tuple_size<T>::value, get_arg_type<F>::arg_size>::value
    >());
}
#endif

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
    void *tuple_offset(size_t i) const {
        return nullptr;
    }
    offset_tuple_impl(const TUPLE *tuple) {
    }
};

template<typename TUPLE>
struct offset_tuple
    : public offset_tuple_impl<TUPLE, std::tuple_size<TUPLE>::value> {
    offset_tuple(const TUPLE *tuple)
        : offset_tuple_impl<TUPLE, std::tuple_size<TUPLE>::value>(tuple) {
    }
};

template<typename TUPLE, std::size_t SIZE>
struct type_tuple_impl {
    template<size_t I_SIZE, std::size_t... I>
    struct type_index_array {
        std::type_index types1_[I_SIZE];

        type_index_array()
            : types1_{ std::type_index(typeid(typename std::tuple_element<I, TUPLE>::type))... } {
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
        return value_.types1_[i];
    }
};

template<typename TUPLE>
struct type_tuple_impl<TUPLE, 0> {
    static const std::size_t size_ = 0;
    const std::type_index tuple_type(size_t i) const {
        return std::type_index(typeid(void));
    }
};

template<typename TUPLE>
struct type_tuple :
    public type_tuple_impl<TUPLE, std::tuple_size<TUPLE>::value> {
    static const std::size_t size_ = std::tuple_size<TUPLE>::value;
};


template<typename TUPLE>
struct remove_reference_tuple {
    static const std::size_t size_ = std::tuple_size<TUPLE>::value;

    template<size_t SIZE, std::size_t... I>
    struct converted {
        typedef std::tuple<typename std::remove_reference<typename std::tuple_element<I, TUPLE>::type>::type...> type;
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
        virtual const std::size_t tuple_size() const = 0;
        virtual const std::type_index tuple_type(size_t i) const = 0;
        virtual void *tuple_element(size_t i) const = 0;

        virtual placeholder * clone() const = 0;
    };

    template<typename ValueType>
    class holder : public placeholder {
    public: // structors
#if 0
        void* operator new(size_t size){
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
    virtual const char * what() const throw() {
        return "bad_any_cast: "
            "failed conversion using any_cast";
    }
};


#if 0
template<typename FUNC, typename TUPLE>
bool can_call_func(const FUNC &func, const TUPLE &tuple) {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    type_tuple<func_arg_type> tuple_func;
    type_tuple<TUPLE> tuple_arg;

    //printf("%d %d\n", tuple_arg.size_, tuple_func.size_);
    if (tuple_arg.size_ < tuple_func.size_)
        return false;
    
    for (size_t i = 0; i < tuple_func.size_; ++i) {
        //printf("%s %s\n", tuple_func.value_.types_[i].name(), tuple_arg.value_.types_[i].name());
        if (tuple_func.value_.types_[i] != tuple_arg.value_.types_[i])
            return false;
    }

    return true;
}
#endif
template<typename FUNC, std::size_t ...I>
auto call_tuple_as_argument(const FUNC &func, const any &arg, const std::index_sequence<I...> &)
    -> typename func_traits<FUNC>::ret_type {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;

    return func(*reinterpret_cast<typename std::tuple_element<I, func_arg_type>::type *>(arg.tuple_element(I))...);
}

template<typename FUNC>
auto call_func(const FUNC &func, const any &arg) -> typename func_traits<FUNC>::ret_type {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    type_tuple<func_arg_type> tuple_func;

    if (arg.tuple_size() < tuple_func.size_)
        throw bad_any_cast();

    for (size_t i = tuple_func.size_; i-- != 0; ) {
        if (arg.tuple_type(i) != tuple_func.tuple_type(i))
            throw bad_any_cast();
    }

    return call_tuple_as_argument(func, arg, std::make_index_sequence<tuple_func.size_>());
}

void test2(int a, char b, volatile const double & c, const char *s) {
    printf("a = %d, %d, %f %s\n", a, (int)b, c, s);
    //return a;
}

int test3() {
    return 3;
}


template<typename TUPLE>
void test1(const TUPLE &t1) {
    //test1_impl(t, std::make_index_sequence<std::tuple_size<TUPLE>::value>());
    //type_tuple<TUPLE> t;

    any a = any(t1);
    //auto can = 
    call_func(&test2, a);
    auto can = call_func(test3, a);
    printf("can = %d\n", (int)can);

    //for (size_t i = 0; i < t.size_; ++i)
    //    printf("%s ", t.value_.types_[i].name());
   // printf("is_standard_layout %d\n", std::is_standard_layout<std::type_index >::value);
}


void test()
{
    auto tuple1 = std::make_tuple(1, 'A', 1.2, "abcd");
#if 0
    using namespace std::literals::string_literals;
    auto result1 = apply(func1, tuple1);
    std::cout << "result1 = " << result1 << std::endl;

    auto tuple2 = std::make_tuple(1, 2);
    auto result2 = apply(func2, tuple2);
    std::cout << "result2 = " << result2 << std::endl;
    
    auto tuple0 = std::make_tuple(1, 2);
    //auto tuple0 = 1;
    auto result0 = apply(func0, tuple0);
    std::cout << "result0 = " << result0 << std::endl;
    
    std::cout << get_arg_type<decltype(&func1)>::arg_size << std::endl;
    std::cout << get_arg_type<decltype(func0)>::arg_size << std::endl;
#endif
    test1(tuple1);
}

int main() {
    test();
    return 0;
}
