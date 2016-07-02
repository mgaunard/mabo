#ifndef MABO_UTILITY_HPP_INCLUDED
#define MABO_UTILITY_HPP_INCLUDED

#include <mabo/config.hpp>
#include <utility>

namespace mabo
{

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

}

#endif
