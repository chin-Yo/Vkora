#pragma once

#include "Traits/FunctionTraits.hpp"
#include "Traits/VariableTraits.hpp"

template <typename T>
struct IsFunction
{
    static constexpr bool value = std::is_function_v<std::remove_pointer_t<T>>
        || std::is_member_function_pointer_v<T>;
};

template <typename T>
constexpr bool IsFunction_Value = IsFunction<T>::value;

template <typename T, bool IsFunc>
struct BasicFieldTraits;

template <typename T>
struct BasicFieldTraits<T, true> : public FunctionTraits<T>
{
    using Traits = FunctionTraits<T>;

    constexpr bool IsMember() const noexcept { return Traits::bIsMember; }
    constexpr bool IsConst() const noexcept { return Traits::bIsConst; }
    constexpr bool IsFunction() const noexcept { return true; }
    constexpr bool IsVariable() const noexcept { return false; }
    constexpr size_t ParamCount() const noexcept { return std::tuple_size_v<typename Traits::Args>; }
};

template <typename T>
struct BasicFieldTraits<T, false> : public VariableTraits<T>
{
    using Traits = VariableTraits<T>;

    constexpr bool IsMember() const noexcept { return Traits::bIsMember; }
    constexpr bool IsFunction() const noexcept { return false; }
    constexpr bool IsVariable() const noexcept { return true; }
};

template <typename T>
struct FieldTraits : public BasicFieldTraits<T, IsFunction_Value<T>>
{
    constexpr FieldTraits(T&& InPointer): Pointer(InPointer)
    {
    }

    T Pointer;
};

template <typename T>
struct ReflInfo;

template <typename T>
constexpr auto GenReflInfo()
{
    return ReflInfo<T>{};
}

template <size_t Idx, typename... Args, typename Class>
void VisitTuple(const std::tuple<Args...>& Tuple, Class* ClassInstance)
{
    using TupleType = std::tuple<Args...>;
    if constexpr (Idx >= std::tuple_size_v<TupleType>)
    {
        return;
    }
    else
    {
        if constexpr (auto elem = std::get<Idx>(Tuple);
            elem.ParamCount() == 1 && std::is_same_v<typename decltype(elem)::Args, std::tuple<int>>)
        {
            (ClassInstance->*elem.Pointer)(3);
        }
        VisitTuple<Idx + 1>(Tuple, ClassInstance);
    }
}

template <size_t Idx = 0, typename... Args, typename Class>
void VisitVarTuple(const std::tuple<Args...>& Tuple, Class* ClassInstance)
{
    using TupleType = std::tuple<Args...>;
    if constexpr (Idx >= std::tuple_size_v<TupleType>)
    {
        return;
    }
    else
    {
        if constexpr (auto elem = std::get<Idx>(Tuple); elem.IsVariable())
        {
            std::cout << (ClassInstance->*elem.Pointer) << std::endl;
        }
        VisitVarTuple<Idx + 1>(Tuple, ClassInstance);
    }
}

#define REFL_INGO(x) template <> struct ReflInfo<x> {

#define VARIABLES(...) static constexpr auto Variables = std::make_tuple(__VA_ARGS__);

#define VAR(v) FieldTraits<decltype(v)>{v}

#define REFL_END() };
