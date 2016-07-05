#ifndef TEST_HPP_INCLUDED
#define TEST_HPP_INCLUDED

namespace testing { namespace internal
{

void PrintTo(const mabo::string_view& bar, ::std::ostream* os) {
  *os << bar;
}

} }

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#endif
