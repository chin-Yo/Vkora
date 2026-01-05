#include "Reflection/JsonSerializer.hpp"
#include <rttr/type>

namespace json_serializer
{
    // ============ 辅助函数 ============

    namespace detail
    {
        // 判断是否为基本类型
        bool is_basic_type(const rttr::type& t)
        {
            return t.is_arithmetic() || t == rttr::type::get<std::string>();
        }

        // 处理基本类型到 JSON
        json basic_type_to_json(const rttr::variant& var)
        {
            auto t = var.get_type();

            if (t == rttr::type::get<bool>())
                return var.to_bool();
            if (t == rttr::type::get<int>())
                return var.to_int();
            if (t == rttr::type::get<int64_t>())
                return var.to_int64();
            if (t == rttr::type::get<uint64_t>())
                return var.to_uint64();
            if (t == rttr::type::get<float>())
                return var.to_float();
            if (t == rttr::type::get<double>())
                return var.to_double();
            if (t == rttr::type::get<std::string>())
                return var.to_string();

            // 其他整数类型
            if (t.is_arithmetic())
            {
                if (var.can_convert<double>())
                    return var.to_double();
            }

            return nullptr;
        }

        // 处理 JSON 到基本类型
        rttr::variant json_to_basic_type(const json& j, const rttr::type& t)
        {
            if (t == rttr::type::get<bool>() && j.is_boolean())
                return j.get<bool>();
            if (t == rttr::type::get<int>() && j.is_number_integer())
                return j.get<int>();
            if (t == rttr::type::get<int64_t>() && j.is_number_integer())
                return j.get<int64_t>();
            if (t == rttr::type::get<uint64_t>() && j.is_number_unsigned())
                return j.get<uint64_t>();
            if (t == rttr::type::get<float>() && j.is_number())
                return j.get<float>();
            if (t == rttr::type::get<double>() && j.is_number())
                return j.get<double>();
            if (t == rttr::type::get<std::string>() && j.is_string())
                return j.get<std::string>();

            return rttr::variant();
        }

        // ============ 序列化变体 ============


        // 处理顺序容器 (vector, list, etc.)
        json sequential_container_to_json(const rttr::variant& var)
        {
            json arr = json::array();
            auto view = var.create_sequential_view();

            for (const auto& item : view)
            {
                arr.push_back(variant_to_json(item.extract_wrapped_value()));
            }

            return arr;
        }

        // 处理关联容器 (map, unordered_map, etc.)
        json associative_container_to_json(const rttr::variant& var)
        {
            json obj = json::object();
            auto view = var.create_associative_view();

            for (const auto& item : view)
            {
                auto key = item.first.extract_wrapped_value();
                auto value = item.second.extract_wrapped_value();

                std::string key_str;
                if (key.can_convert<std::string>())
                {
                    key_str = key.to_string();
                }
                else
                {
                    key_str = key.to_string(); // RTTR 会尝试转换
                }

                obj[key_str] = variant_to_json(value);
            }

            return obj;
        }

        // 处理类/结构体
        json object_to_json(const rttr::instance& obj)
        {
            json result = json::object();

            rttr::instance wrapped = obj.get_type().get_raw_type().is_wrapper()
                                         ? obj.get_wrapped_instance()
                                         : obj;

            auto t = wrapped.get_derived_type();

            for (auto& prop : t.get_properties())
            {
                rttr::variant value = prop.get_value(wrapped);

                if (!value.is_valid()) continue;

                json prop_json = variant_to_json(value);
                if (!prop_json.is_null())
                {
                    result[prop.get_name().to_string()] = prop_json;
                }
            }

            return result;
        }

        // 主序列化函数
        json variant_to_json(const rttr::variant& var)
        {
            if (!var.is_valid()) return nullptr;

            auto t = var.get_type();
            auto wrapped_type = t.is_wrapper() ? t.get_wrapped_type() : t;

            // 基本类型
            if (is_basic_type(wrapped_type))
            {
                return basic_type_to_json(var);
            }

            // 枚举类型
            if (t.is_enumeration())
            {
                bool ok = false;
                auto str = t.get_enumeration().value_to_name(var).to_string();
                return str.empty() ? json(var.to_int64()) : json(str);
            }

            // std::optional 处理
            if (t.is_wrapper() && t.get_wrapped_type().is_valid())
            {
                auto wrapped = var.extract_wrapped_value();
                if (!wrapped.is_valid())
                {
                    return nullptr; // optional 为空
                }
                return variant_to_json(wrapped);
            }

            // 顺序容器
            if (var.is_sequential_container())
            {
                return sequential_container_to_json(var);
            }

            // 关联容器
            if (var.is_associative_container())
            {
                return associative_container_to_json(var);
            }

            // 复杂对象
            if (t.get_properties().size() > 0)
            {
                return object_to_json(var);
            }

            return nullptr;
        }

        // ============ 反序列化变体 ============
        bool json_to_variant(const json& j, rttr::variant& var, const rttr::type& t);

        // 处理顺序容器
        bool json_to_sequential_container(const json& j, rttr::variant& var, const rttr::type& t)
        {
            if (!j.is_array()) return false;

            var = t.create();
            if (!var.is_valid()) return false;

            auto view = var.create_sequential_view();
            auto value_type = view.get_value_type();

            view.set_size(j.size());

            size_t i = 0;
            for (const auto& item : j)
            {
                rttr::variant item_var;
                if (json_to_variant(item, item_var, value_type))
                {
                    view.set_value(i, item_var);
                }
                ++i;
            }

            return true;
        }

        // 处理关联容器
        bool json_to_associative_container(const json& j, rttr::variant& var, const rttr::type& t)
        {
            if (!j.is_object()) return false;

            var = t.create();
            if (!var.is_valid()) return false;

            auto view = var.create_associative_view();
            const auto key_type = view.get_key_type();
            auto value_type = view.get_value_type();

            for (auto& [key, value] : j.items())
            {
                rttr::variant key_var = key;
                rttr::variant value_var;

                if (!key_var.convert(key_type)) continue;
                if (!json_to_variant(value, value_var, value_type)) continue;

                view.insert(key_var, value_var);
            }

            return true;
        }

        // 处理对象
        bool json_to_object(const json& j, rttr::instance obj)
        {
            if (!j.is_object()) return false;

            rttr::instance wrapped = obj.get_type().get_raw_type().is_wrapper()
                                         ? obj.get_wrapped_instance()
                                         : obj;

            auto t = wrapped.get_derived_type();

            for (auto& prop : t.get_properties())
            {
                const auto& name = prop.get_name().to_string();

                if (!j.contains(name)) continue;

                const auto& prop_json = j[name];
                if (prop_json.is_null()) continue;

                rttr::variant prop_value;
                if (json_to_variant(prop_json, prop_value, prop.get_type()))
                {
                    prop.set_value(wrapped, prop_value);
                }
            }

            return true;
        }

        // 主反序列化函数
        bool json_to_variant(const json& j, rttr::variant& var, const rttr::type& t)
        {
            if (j.is_null()) return false;

            auto wrapped_type = t.is_wrapper() ? t.get_wrapped_type() : t;

            // 基本类型
            if (is_basic_type(wrapped_type))
            {
                var = json_to_basic_type(j, wrapped_type);
                if (var.is_valid() && t.is_wrapper())
                {
                    var.convert(t);
                }
                return var.is_valid();
            }

            // 枚举类型
            if (t.is_enumeration())
            {
                if (j.is_string())
                {
                    var = t.get_enumeration().name_to_value(j.get<std::string>());
                }
                else if (j.is_number_integer())
                {
                    var = j.get<int64_t>();
                    var.convert(t);
                }
                return var.is_valid();
            }

            // 顺序容器
            if (t.is_sequential_container())
            {
                return json_to_sequential_container(j, var, t);
            }

            // 关联容器
            if (t.is_associative_container())
            {
                return json_to_associative_container(j, var, t);
            }

            // 复杂对象
            if (t.get_properties().size() > 0)
            {
                var = t.create();
                if (!var.is_valid()) return false;
                return json_to_object(j, var);
            }

            return false;
        }
    } // namespace detail

    // ============ 公共接口实现 ============

    json to_json(const rttr::instance& obj)
    {
        return detail::object_to_json(obj);
    }

    bool from_json(const json& j, rttr::instance obj)
    {
        return detail::json_to_object(j, obj);
    }
} // namespace json_serializer
