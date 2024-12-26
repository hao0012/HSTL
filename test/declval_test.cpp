#include <gtest/gtest.h>

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
  //  constructor
  decltype(std::declval<NonDefault>().foo()) n2 = n1;  // type of n2 is int
}