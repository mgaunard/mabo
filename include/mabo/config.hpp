#ifndef MABO_CONFIG_HPP_INCLUDED
#define MABO_CONFIG_HPP_INCLUDED

#include <vector>
#include <string>
#include <utility>
#include <cstddef>
#include <experimental/variant.hpp>
#include <experimental/string_view>
#include <experimental/optional>
#include <range/v3/iterator_range.hpp>
#include <iostream>

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
using variant = std::experimental::variant<T...>;

template<class T>
using optional = std::experimental::optional<T>;

}

#endif
