#ifndef TYPE_TRAITS_HPP_
#define TYPE_TRAITS_HPP_

#include <cstddef>

namespace hstl {

// ------------------- remove_reference ------------------- //
template <typename T>
struct remove_reference {
  using type = T;
};

template <typename T>
struct remove_reference<T&> {
  using type = T;
};

template <typename T>
struct remove_reference<T&&> {
  using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

// --------------------- enable_if ----------------------- //
template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
  using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

// ---------------- add_rvalue_reference ------------------ //
// "cv void&"时该模板替换失败，跳到下一个模板(SFINAE)
template <typename T>
T&& try_add_rvalue_reference(int);

// 处理 T = cv void 的情况
template <typename T>
T try_add_rvalue_reference(...);

template <typename T>
struct add_rvalue_reference {
  // 优先匹配第一个try_add_rvalue_reference
  using type = decltype(try_add_rvalue_reference<T>(0));
};

template <typename T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

/**
 *  以下实现无法处理 cv void 的情况，两个模板都无法匹配
 *  template<typename T>
 *  struct add_rvalue_reference {
 *    using type = T&&;
 *  };
 *  template<typename T>
 *  struct add_rvalue_reference<T&> {
 *    using type = T&&;
 *  };
 */

// ---------------- add_lvalue_reference ------------------ //
// "cv void&"时该模板替换失败，跳到下一个模板(SFINAE)
template <typename T>
T& try_add_lvalue_reference(int);

// 处理 T = cv void 的情况
template <typename T>
T try_add_lvalue_reference(...);

template <typename T>
struct add_lvalue_reference {
  using type = decltype(try_add_lvalue_reference<T>(0));
};

template <typename T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

// ------------------ integral_constant ------------------ //
// 以T为类型、值为v的整数常量
template <typename T, T v>
struct integral_constant {
  using value_type = T;
  using type = integral_constant;
  static constexpr T value = v;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

// ---------------- is_lvalue_reference ------------------ //
template <typename T>
struct is_lvalue_reference : false_type {};
template <typename T>
struct is_lvalue_reference<T&> : true_type {};

template <typename T>
constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

// ---------------- is_rvalue_reference ------------------ //
template <typename T>
struct is_rvalue_reference : false_type {};
template <typename T>
struct is_rvalue_reference<T&&> : true_type {};

template <typename T>
constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;

// ---------------- is_reference ------------------ //
template <typename T>
struct is_reference : false_type {};
template <typename T>
struct is_reference<T&> : true_type {};
template <typename T>
struct is_reference<T&&> : true_type {};

template <typename T>
constexpr bool is_reference_v = is_reference<T>::value;

// ---------------- is_convertible ------------------ //
// 用于判断From是否可以转换为To
template <typename From, typename To>
struct is_convertible : false_type {};

// ---------------- is_same ------------------ //
template <typename T, typename U>
struct is_same : false_type {};

template <typename T>
struct is_same<T, T> : true_type {};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

// ---------------- is_pointer ------------------ //
template <typename T>
struct is_pointer : false_type {};

template <typename T>
struct is_pointer<T*> : true_type {};

template <typename T>
struct is_pointer<T* const> : true_type {};

template <typename T>
struct is_pointer<T* volatile> : true_type {};

template <typename T>
struct is_pointer<T* const volatile> : true_type {};

template <typename T>
constexpr bool is_pointer_v = is_pointer<T>::value;

// ---------------- is_array ------------------ //
template <typename T>
struct is_array : false_type {};

template <typename T>
struct is_array<T[]> : true_type {};

template <typename T, size_t N>
struct is_array<T[N]> : true_type {};

template<typename T>
constexpr bool is_array_v = is_array<T>::value;

// ---------------- conditional ------------------ //
template <bool B, typename T, typename F>
struct conditional {
  using type = F;
};

template <typename T, typename F>
struct conditional<true, T, F> {
  using type = T;
};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// ---------------- void_t ------------------ //
template<typename... >
using void_t = void;

}  // namespace hstl

#endif  // TYPE_TRAITS_HPP_