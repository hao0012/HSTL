#include <gtest/gtest.h>

#include <iostream>

#include "utility.hpp"

void inc(int& v) { ++v; }

void inc2(int&& v) { ++v; }

template <typename F, typename T>
void flip1(F f, T&& t) {
  f(hstl::forward<T>(t));
}

TEST(ForwardTest, TEST1) {
  int i = 10;
  inc(i);
  EXPECT_EQ(i, 11);
  flip1(inc, i);
  EXPECT_EQ(i, 12);

  flip1(inc2, 10);  // 编译不报错即可
}