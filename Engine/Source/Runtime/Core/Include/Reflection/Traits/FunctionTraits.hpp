#pragma once
#include <tuple>

namespace detail
{
    template <typename Func>
    struct BasicFunctionTraits;

    template <typename Ret, typename... Args>
    struct BasicFunctionTraits<Ret(Args...)>
    {
        using Params = std::tuple<Args...>;
        using ReturnType = Ret;
    };
}

// Primary template
template <typename Func>
struct FunctionTraits;

// Partial specialization : Ordinary function
template <typename Ret, typename... Args>
struct FunctionTraits<Ret(Args...)> : detail::BasicFunctionTraits<Ret(Args...)>
{
    using Type = Ret(Args...);
    using ClassWithArgs = std::tuple<Args...>;
    using Pointer = Ret(*)(Args...);
    static constexpr bool bIsMember = false;
    static constexpr bool bIsConst = false;
};

// Partial specialization : Class member function
template <typename Ret, typename Class, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...)> : detail::BasicFunctionTraits<Ret(Args...)>
{
    using Type = Ret (Class::*)(Args...);
    using ClassWithArgs = std::tuple<Class*, Args...>;
    using Pointer = Ret (Class::*)(Args...);
    static constexpr bool bIsMember = true;
    static constexpr bool bIsConst = false;
};

// Partial specialization : Class member function, and the return value is const
template <typename Ret, typename Class, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...) const> : detail::BasicFunctionTraits<Ret(Args...)>
{
    using Type = Ret (Class::*)(Args...) const;
    using ClassWithArgs = std::tuple<Class*, Args...>;
    using Pointer = Ret (Class::*)(Args...) const;
    static constexpr bool bIsMember = true;
    static constexpr bool bIsConst = true;
};
