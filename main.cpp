#include <vector>
#include <string>
#include <utility>
#include <cstddef>
#include <experimental/variant.hpp>
#include <experimental/string_view>
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

template<class... T>
using variant = std::experimental::variant<T...>;

template<class F0, class F1>
struct overloaded : F0, F1
{
    overloaded(F0&& f0, F1&& f1) : F0(f0), F1(f1) {}

    using F0::operator();
    using F1::operator();
};

template<class F0, class F1>
overloaded<F0, F1> overload(F0&& f0, F1&& f1)
{
    return overloaded<F0, F1>(std::forward<F0>(f0), std::forward<F1>(f1));
}

struct symbol
{
};

struct section
{
    string name;
};

struct object
{
    string name;
    vector<section> sections;
};

// collection of objects, but only load as needed
struct archive
{
    string name;
    vector<pair<object, bool>> objects;
};

using binary = variant<object, archive>;

}

#include "radare2.hpp"

int main(int argc, char* argv[])
{
    mabo::context ctx;
    for(const char* arg : ranges::make_iterator_range(argv+1, argv+argc))
        ctx.load_file(arg);
#if 0
    std::cout << "undefined symbols:\n";
    for(mabo::string_view s : ctx.undefined_symbols())
    {
        std::cout << s << "\n";
    }
#endif
}