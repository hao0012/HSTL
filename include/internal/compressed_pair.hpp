#ifndef COMPRESSED_PAIR_HPP_
#define COMPRESSED_PAIR_HPP_

#include <type_traits>

#include "call_traits.hpp"

namespace hstl {

template <typename T1, typename T2>
class compressed_pair;

/**
 * @brief 根据T1和T2是否相同、为空，来判断应该使用compressed_pair_imp的哪个版本
 */
template <typename T1, typename T2, bool isSame, bool firstEmpty,
          bool secondEmpty>
struct compressed_pair_switch;

template <typename T1, typename T2>
struct compressed_pair_switch<T1, T2, false, false, false> {
  static const int value = 0;
};

template <typename T1, typename T2>
struct compressed_pair_switch<T1, T2, false, true, false> {
  static const int value = 1;
};

template <typename T1, typename T2>
struct compressed_pair_switch<T1, T2, false, false, true> {
  static const int value = 2;
};

template <typename T1, typename T2>
struct compressed_pair_switch<T1, T2, false, true, true> {
  static const int value = 3;
};

template <typename T1, typename T2>
struct compressed_pair_switch<T1, T2, true, true, true> {
  static const int value = 4;
};

template <typename T1, typename T2>
struct compressed_pair_switch<T1, T2, true, false, false> {
  static const int value = 5;
};

template <typename T1, typename T2>
constexpr int compressed_pair_switch_v =
    compressed_pair_switch<T1, T2, is_same_v<T1, T2>,
                           std::is_empty<std::remove_cv_t<T1>>::value,
                           std::is_empty<std::remove_cv_t<T2>>::value>::value;

template <typename T1, typename T2, int version>
class compressed_pair_imp;

// 以下各个compressed_pair_imp版本与compressed_pair_switch的value对应
// 采用EBO实现，private继承空的T，使其不占用空间

/**
 * @brief T1和T2不同，且都不为空
 */
template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 0> {
 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  compressed_pair_imp() {}

  compressed_pair_imp(first_param_type x, second_param_type y)
      : first_(x), second_(y) {}

  compressed_pair_imp(first_param_type x) : first_(x) {}

  compressed_pair_imp(second_param_type y) : second_(y) {}

  first_reference first() { return first_; }
  first_const_reference first() const { return first_; }

  second_reference second() { return second_; }
  second_const_reference second() const { return second_; }

  void swap(compressed_pair<T1, T2>& y) {
    cp_swap(first_, y.first());
    cp_swap(second_, y.second());
  }

 private:
  first_type first_;
  second_type second_;
};

/**
 * @brief T1和T2不同，且T1为空
 */
template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 1> : private T1 {
 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  compressed_pair_imp() {}

  compressed_pair_imp(first_param_type x, second_param_type y)
      : first_type(x) /* 调用父类构造函数初始化 */, second_(y) {}

  compressed_pair_imp(first_param_type x) : first_type(x) {}

  compressed_pair_imp(second_param_type y) : second_(y) {}

  // 当前类继承了T1，所以这里直接返回this
  first_reference first() { return *this; }
  first_const_reference first() const { return *this; }

  second_reference second() { return second_; }
  second_const_reference second() const { return second_; }

  void swap(compressed_pair<T1, T2>& y) { cp_swap(second_, y.second()); }

 private:
  second_type second_;
};

/**
 * @brief T1和T2不同，且T2为空
 */
template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 2> : private T2 {
 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  compressed_pair_imp() {}

  compressed_pair_imp(first_param_type x, second_param_type y)
      : second_type(y), first_(x) {}

  compressed_pair_imp(first_param_type x) : first_(x) {}

  compressed_pair_imp(second_param_type y) : second_type(y) {}

  first_reference first() { return first_; }
  first_const_reference first() const { return first_; }

  second_reference second() { return *this; }
  second_const_reference second() const { return *this; }

  void swap(compressed_pair<T1, T2>& y) { cp_swap(first_, y.first()); }

 private:
  first_type first_;
};

/**
 * @brief T1和T2类型不同，且都为空
 */
template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 3> : private T1, T2 {
 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  compressed_pair_imp() {}

  compressed_pair_imp(first_param_type x, second_param_type y)
      : first_type(x), second_type(y) {}

  compressed_pair_imp(first_param_type x) : first_type(x) {}

  compressed_pair_imp(second_param_type y) : second_type(y) {}

  first_reference first() { return *this; }
  first_const_reference first() const { return *this; }

  second_reference second() { return *this; }
  second_const_reference second() const { return *this; }

  void swap(compressed_pair<T1, T2>& y) {}
};

/**
 * @brief T1和T2类型相同，且都为空
 */
template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 4> : private T1 {
 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  compressed_pair_imp() {}

  compressed_pair_imp(first_param_type x, second_param_type) : first_type(x) {}

  compressed_pair_imp(first_param_type x) : first_type(x) {}

  first_reference first() { return *this; }
  first_const_reference first() const { return *this; }

  second_reference second() { return *this; }
  second_const_reference second() const { return *this; }

  void swap(compressed_pair<T1, T2>& y) {}
};

/**
 * @brief T1和T2类型相同，且都不为空
 */
template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 5> {
 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  compressed_pair_imp() {}

  compressed_pair_imp(first_param_type x, second_param_type y)
      : first_(x), second_(y) {}

  compressed_pair_imp(first_param_type x) : first_(x) {}
  // 没有second_的构造函数，因为T1和T2类型相同，会重复定义

  first_reference first() { return first_; }
  first_const_reference first() const { return first_; }

  second_reference second() { return second_; }
  second_const_reference second() const { return second_; }

  void swap(compressed_pair<T1, T2>& y) {
    cp_swap(first_, y.first());
    cp_swap(second_, y.second());
  }

 private:
  first_type first_;
  second_type second_;
};

template <class T1, class T2>
class compressed_pair
    : private compressed_pair_imp<T1, T2, compressed_pair_switch_v<T1, T2>> {
 private:
  using base = compressed_pair_imp<T1, T2, compressed_pair_switch_v<T1, T2>>;

 public:
  using first_type = T1;
  using second_type = T2;
  using first_param_type = typename call_traits<first_type>::param_type;
  using second_param_type = typename call_traits<second_type>::param_type;
  using first_reference = typename call_traits<first_type>::reference;
  using second_reference = typename call_traits<second_type>::reference;
  using first_const_reference =
      typename call_traits<first_type>::const_reference;
  using second_const_reference =
      typename call_traits<second_type>::const_reference;

  // 需要再次定义，无法直接使用从基类继承的所有构造函数，必须在这里写一遍

  compressed_pair() : base() {}
  compressed_pair(first_param_type x, second_param_type y) : base(x, y) {}
  explicit compressed_pair(first_param_type x) : base(x) {}
  explicit compressed_pair(second_param_type y) : base(y) {}

  first_reference first() { return base::first(); }
  first_const_reference first() const { return base::first(); }

  second_reference second() { return base::second(); }
  second_const_reference second() const { return base::second(); }

  void swap(compressed_pair& y) { base::swap(y); }
};

}  // namespace hstl

#endif  // COMPRESSED_PAIR_HPP_