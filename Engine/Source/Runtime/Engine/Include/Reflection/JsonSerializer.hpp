#pragma once
#include <rttr/type>
#include <nlohmann/json.hpp>
#include <optional>

namespace json_serializer
{
    using json = nlohmann::json;

    // ============ 前向声明 ============
    json to_json(const rttr::instance& obj);
    bool from_json(const json& j, rttr::instance obj);

    // ============ 序列化接口 ============

    /**
     * @brief 将任意 RTTR 注册的对象序列化为 JSON
     */
    template <typename T>
    json serialize(const T& obj)
    {
        return to_json(rttr::instance(obj));
    }

    namespace detail
    {
        json variant_to_json(const rttr::variant& var);
    }

    /**
     * @brief 将 JSON 反序列化为对象
     */
    template <typename T>
    std::optional<T> deserialize(const json& j)
    {
        T obj{};
        if (from_json(j, obj))
        {
            return obj;
        }
        return std::nullopt;
    }

    /**
     * @brief 将 JSON 反序列化到已存在的对象
     */
    template <typename T>
    bool deserialize_to(const json& j, T& obj)
    {
        return from_json(j, obj);
    }

    // ============ 便捷函数 ============

    template <typename T>
    std::string to_string(const T& obj, int indent = -1)
    {
        return serialize(obj).dump(indent);
    }

    template <typename T>
    std::optional<T> from_string(const std::string& str)
    {
        try
        {
            return deserialize<T>(json::parse(str));
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
} // namespace json_serializer
