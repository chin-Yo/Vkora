#pragma once
#include <type_traits>

namespace detail
{
    // Primary template: handles general types (non-member-pointer cases)
    template <typename T>
    struct VariableType
    {
        // For normal types, Type is just T itself
        using Type = T;
    };

    // Partial specialization: handles pointer-to-member types (T Class::* syntax)
    template <typename Class, typename T>
    struct VariableType<T Class::*>
    {
        // Extracts the underlying member type (T) from the member pointer.
        using Type = T;
    };
}

template <typename T>
using VariableType_Type = typename detail::VariableType<T>::Type;

namespace internal
{
    template <typename T>
    struct BasicVariableTraits
    {
        using Type = VariableType_Type<T>;
        static constexpr bool bIsMember = std::is_member_pointer_v<T>;
    };
}

template <typename T>
struct VariableTraits;

template <typename T>
struct VariableTraits<T*> : internal::BasicVariableTraits<T>
{
    using PointerType = T*;
};

template <typename Class, typename T>
struct VariableTraits<T Class::*> : internal::BasicVariableTraits<T Class::*>
{
    using PointerType = T Class::*;
    using ClassType = Class;
};
