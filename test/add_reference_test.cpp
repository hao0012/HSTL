#include <gtest/gtest.h>

#include "type_traits.hpp"

TEST(IS_REFERENCE_TEST, TEST1) {
  using non_ref = int;
  ASSERT_EQ(hstl::is_lvalue_reference_v<non_ref>, false);

  using l_ref = hstl::add_lvalue_reference_t<non_ref>;
  ASSERT_EQ(hstl::is_lvalue_reference_v<l_ref>, true);

  using r_ref = hstl::add_rvalue_reference_t<non_ref>;
  ASSERT_EQ(hstl::is_rvalue_reference_v<r_ref>, true);

  using void_ref = hstl::add_lvalue_reference_t<void>;
  ASSERT_EQ(hstl::is_reference_v<void_ref>, false);

  using l_ref2 = hstl::add_lvalue_reference_t<l_ref>;
  ASSERT_EQ(hstl::is_lvalue_reference_v<l_ref2>, true);

  using l_ref3 = hstl::add_rvalue_reference_t<l_ref>;
  ASSERT_EQ(hstl::is_lvalue_reference_v<l_ref3>, true);

  using r_ref2 = hstl::add_rvalue_reference_t<r_ref>;
  ASSERT_EQ(hstl::is_rvalue_reference_v<r_ref2>, true);

  using r_ref3 = hstl::add_lvalue_reference_t<r_ref>;
  ASSERT_EQ(hstl::is_lvalue_reference_v<r_ref3>, true);
}