#ifndef MABO_CONFIG_HPP_INCLUDED
#define MABO_CONFIG_HPP_INCLUDED

// mpark
#include <experimental/variant.hpp>

// libstdc++
#include <experimental/string_view>
#include <experimental/optional>

#include <vector>
#include <string>
#include <utility>

namespace mabo
{

template<class T>
using vector = std::vector<T>;

using string = std::string;
using string_view = std::experimental::string_view;

template<class T0, class T1>
using pair = std::pair<T0, T1>;

#define MABO_VARIANT_NS std::experimental
template<class... T>
using variant = MABO_VARIANT_NS::variant<T...>;
using MABO_VARIANT_NS::get;
using MABO_VARIANT_NS::holds_alternative;
using MABO_VARIANT_NS::visit;

template<class T>
using optional = std::experimental::optional<T>;


template<class T>
size_t hash_value(T const& t);

namespace detail
{

struct hash_value
{
    template<class T>
    size_t operator()(T const& t) const
    {
        using ::mabo::hash_value;
        return hash_value(t);
    }
};

}

}

#endif
