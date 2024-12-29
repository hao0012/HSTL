#include <gtest/gtest.h>

#include "type_traits.hpp"
#include "utility.hpp"

struct Default {
  int foo() const { return 1; }
};

struct NonDefault {
  NonDefault() = delete;
  int foo() const { return 1; }
};

TEST(DECLVAL_TEST, TEST1) {
  decltype(Default().foo()) n1 = 1;  // type of n1 is int
  //  decltype(NonDefault().foo()) n2 = n1;             // error: no default
  decltype(hstl::declval<NonDefault>().foo()) n2 = n1;  // type of n2 is int
  ASSERT_TRUE((hstl::is_same_v<decltype(n2), int>));
}