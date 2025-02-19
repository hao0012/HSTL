#include <gtest/gtest.h>
#include "function.hpp"

int add(int a, int b) { return a + b; }

struct Add {
  int operator()(int a, int b) {
    return a + b;
  }
};

TEST(FUNCTION_TEST, TEST1) {
  hstl::function<int(int, int)> f1(add);
  ASSERT_EQ(f1(1, 2), 3);
  auto add_lambda = [](int a, int b) { return a + b; };
  hstl::function<int(int, int)> f2(add_lambda);
  ASSERT_EQ(f2(3, 4), 7);
  hstl::function<int(int, int)> f3(Add{});
  ASSERT_EQ(f3(5, 6), 11);

  int (*p)(int, int) = add;
  hstl::function<int(int, int)> f4(p);
  ASSERT_EQ(f4(7, 8), 15);
}