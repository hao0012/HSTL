#ifndef CALL_TRAITS_HPP_
#define CALL_TRAITS_HPP_

#include <type_traits>

#include "type_traits.hpp"

namespace hstl {

/**
 * @brief
 * 根据T的大小来选择参数类型，如果小于等于void*的大小，则选择值传递，否则选择引用传递
 */
template <typename T, bool small_>
struct ct_imp2 {
  using param_type = const T&;
};
template <typename T>
struct ct_imp2<T, true> {
  using param_type = const T;
};

/**
 * @param isp T是否是指针
 * @param b1  T是否是算术类型
 */
template <typename T, bool isp, bool b1>
struct ct_imp {
  using param_type = const T&;
};

/**
 * @brief T是算术类型，根据T的大小选择值或引用类型
 */
template <typename T, bool isp>
struct ct_imp<T, isp, true> {
  using param_type =
      typename ct_imp2<T, sizeof(T) <= sizeof(void*)>::param_type;
};

/**
 * @brief T是指针类型，返回const T
 */
template <typename T, bool b1>
struct ct_imp<T, true, b1> {
  using param_type = T const;
};

/**
 * 在函数参数传递时，根据实际的参数选择值或引用传递，程序员可以不用考虑究竟应该选择哪种传入方式
 */
template <typename T>
struct call_traits {
 public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  // TODO(hao): is_arithmetic
  using param_type = typename ct_imp<T, is_pointer<T>::value,
                                     std::is_arithmetic<T>::value>::param_type;
};

/**
 * @brief 如果是引用类型，不需要优化
 */
template <typename T>
struct call_traits<T&> {
  using value_type = T&;
  using reference = T&;
  using const_reference = const T&;
};

// TODO(hao): call_traits<T[]>

}  // namespace hstl

#endif  // CALL_TRAITS_HPP_