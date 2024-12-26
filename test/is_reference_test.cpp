#include <gtest/gtest.h>

#include <iostream>
#include <type_traits>

#include "type_traits.hpp"

TEST(IS_LVALUE_REFERENCE_TEST, TEST1) {
  // TODO(hao): 为什么这里会报错
  // ASSERT_EQ(hstl::is_lvalue_reference<int&>::value, true);
  ASSERT_EQ(hstl::is_lvalue_reference_v<int&>, true);
  ASSERT_EQ(hstl::is_lvalue_reference_v<int&&>, false);
  ASSERT_EQ(hstl::is_lvalue_reference_v<int>, false);

  ASSERT_EQ(hstl::is_lvalue_reference_v<void>, false);

  class A {};
  ASSERT_EQ(hstl::is_lvalue_reference_v<A>, false);
  ASSERT_EQ(hstl::is_lvalue_reference_v<A&>, true);
  ASSERT_EQ(hstl::is_lvalue_reference_v<A&&>, false);
}

TEST(IS_RVALUE_REFERENCE_TEST, TEST1) {
  ASSERT_EQ(hstl::is_rvalue_reference_v<int&>, false);
  ASSERT_EQ(hstl::is_rvalue_reference_v<int&&>, true);
  ASSERT_EQ(hstl::is_rvalue_reference_v<int>, false);

  ASSERT_EQ(hstl::is_rvalue_reference_v<void>, false);

  class A {};
  ASSERT_EQ(hstl::is_rvalue_reference_v<A>, false);
  ASSERT_EQ(hstl::is_rvalue_reference_v<A&>, false);
  ASSERT_EQ(hstl::is_rvalue_reference_v<A&&>, true);
}

TEST(IS_REFERENCE_TEST, TEST1) {
  ASSERT_EQ(hstl::is_reference_v<int&>, true);
  ASSERT_EQ(hstl::is_reference_v<int&&>, true);
  ASSERT_EQ(hstl::is_reference_v<int>, false);

  ASSERT_EQ(hstl::is_reference_v<void>, false);

  class A {};
  ASSERT_EQ(hstl::is_reference_v<A>, false);
  ASSERT_EQ(hstl::is_reference_v<A&>, true);
  ASSERT_EQ(hstl::is_reference_v<A&&>, true);
}

template <typename T>
void test(T&& x) {
  // ASSERT_EQ(std::is_same_v<T&&, decltype(x)>);
  std::cout << "T\t" << hstl::is_rvalue_reference<T>::value << '\n';
  std::cout << "T&&\t" << hstl::is_rvalue_reference<T&&>::value << '\n';
  std::cout << "decltype(x)\t" << hstl::is_rvalue_reference<decltype(x)>::value
            << '\n';
  std::cout << "decltype((x))\t"
            << hstl::is_rvalue_reference<decltype((x))>::value << '\n';
  // 下面是正确答案
  std::cout << "T\t" << std::is_rvalue_reference<T>::value << '\n';
  std::cout << "T&&\t" << std::is_rvalue_reference<T&&>::value << '\n';
  std::cout << "decltype(x)\t" << std::is_rvalue_reference<decltype(x)>::value
            << '\n';
  std::cout << "decltype((x))\t"
            << std::is_rvalue_reference<decltype((x))>::value << '\n';
}

TEST(IS_RVALUE_REFERENCE_TEST, TEST2) {
  std::cout << "\ntest(42)\n";
  test(42);

  std::cout << "\ntest(x)\n";
  int x = 42;
  test(x);
}