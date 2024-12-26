#ifndef UTILITY_HPP_
#define UTILITY_HPP_

#include "type_traits.hpp"

namespace hstl {

template <typename T>
constexpr T&& forward(hstl::remove_reference_t<T>& t) noexcept {
  return static_cast<T&&>(t);
}

template <typename T>
constexpr T&& forward(hstl::remove_reference_t<T>&& t) noexcept {
  return static_cast<T&&>(t);
}

template <typename T>
constexpr hstl::remove_reference_t<T>&& move(T&& t) noexcept {
  return static_cast<hstl::remove_reference_t<T>&&>(t);
}

// 如果T为左值引用，则返回左值引用，如果T为右值引用或值类型，则返回右值引用
// 专门用于decltype中没有默认构造函数的类
template <typename T>
hstl::add_rvalue_reference_t<T> declval() noexcept;

}  // namespace hstl

#endif  // UTILITY_HPP_