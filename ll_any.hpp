// See http://www.boost.org/libs/any for Documentation.

#ifndef LL_ANY_HPP_
#define LL_ANY_HPP_

// what:  variant type ll::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

#include <algorithm>
#include <typeinfo>

namespace ll
{
    template<typename T>
    struct remove_reference {
        typedef T type;
    };
    template<typename T>
    struct remove_reference<T&> {
        typedef T type;
    };
    template<typename T>
    struct remove_reference<const T&> {
        typedef T type;
    };

    template<class BaseT = void>
    class any
    {
    public: // structors

        any()
          : content(0)
        {
        }

        template<typename ValueType>
        any(const ValueType & value)
          : content(new holder<ValueType>(value))
        {
        }

        any(const any & other)
          : content(other.content ? other.content->clone() : 0)
        {
        }

        ~any()
        {
            delete content;
        }

    public: // modifiers

        any & swap(any & rhs)
        {
            std::swap(content, rhs.content);
            return *this;
        }

        template<typename ValueType>
        any & operator=(const ValueType & rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

        any & operator=(const any & rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

    public: // queries

        bool empty() const
        {
            return !content;
        }

        const std::type_info & type() const
        {
            return content ? content->type() : typeid(void);
        }

    public: // types (public so any_cast can be non-friend)

        class placeholder
        {
        public: // structors

            virtual ~placeholder()
            {
            }

        public: // queries

            virtual const std::type_info & type() const = 0;

            virtual placeholder * clone() const = 0;
            
            virtual BaseT *get_pointer() = 0;

        };

        template<typename ValueType>
        class holder : public placeholder
        {
        public: // structors

            holder(const ValueType & value)
              : held(value)
            {
            }

        public: // queries

            virtual const std::type_info & type() const
            {
                return typeid(ValueType);
            }

            virtual placeholder * clone() const
            {
                return new holder(held);
            }
            
            inline virtual BaseT *get_pointer()
            {
                return static_cast<BaseT *>(&held);
            }

        public: // representation

            ValueType held;

        private: // intentionally left unimplemented
            holder & operator=(const holder &);
        };

    public: // representation (public so any_cast can be non-friend)

        placeholder * content;

	    BaseT *base()
	    {
		    return content->get_pointer();
	    }
	    const BaseT *base() const
	    {
		    return content->get_pointer();
	    }
    };
    
    class bad_any_cast : public std::bad_cast
    {
    public:
        virtual const char * what() const throw()
        {
            return "ll::bad_any_cast: "
                   "failed conversion using ll::any_cast";
        }
    };

    template<typename ValueType, class BaseT>
    ValueType * any_cast(any<BaseT> * operand)
    {
        typedef typename any<BaseT>::template holder<ValueType> holder_t;
        return operand &&
            operand->type() == typeid(ValueType)
            ? &static_cast<holder_t *>(operand->content)->held
            : 0;
    }

    template<typename ValueType, class BaseT>
    inline const ValueType * any_cast(const any<BaseT> * operand)
    {
        return any_cast<ValueType>(const_cast<any<BaseT> *>(operand));
    }

    template<typename ValueType, class BaseT>
    ValueType any_cast(any<BaseT> & operand)
    {
        typedef typename remove_reference<ValueType>::type nonref;

        nonref * result = any_cast<nonref>(&operand);
        if(!result)
            throw(bad_any_cast());
        return *result;
    }

    template<typename ValueType, class BaseT>
    inline ValueType any_cast(const any<BaseT> & operand)
    {
        typedef typename remove_reference<ValueType>::type nonref;
        return any_cast<const nonref &>(const_cast<any<BaseT> &>(operand));
    }

    // Note: The "unsafe" versions of any_cast are not part of the
    // public interface and may be removed at any time. They are
    // required where we know what type is stored in the any and can't
    // use typeid() comparison, e.g., when our types may travel across
    // different shared libraries.
    template<typename ValueType, class BaseT>
    inline ValueType * unsafe_any_cast(any<BaseT> * operand)
    {
        typedef typename any<BaseT>::template holder<ValueType> holder_t;    
        return &static_cast<holder_t *>(operand->content)->held;
    }

    template<typename ValueType, class BaseT>
    inline const ValueType * unsafe_any_cast(const any<BaseT> * operand)
    {
        return unsafe_any_cast<ValueType>(const_cast<any<BaseT> *>(operand));
    }
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#endif
