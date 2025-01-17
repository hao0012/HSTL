#ifndef ITERATOR_HPP_
#define ITERATOR_HPP_

#include <cstddef>
#include "type_traits.hpp"

namespace hstl {

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};
// C++20
// struct contiguous_iterator_tag : public random_access_iterator_tag {};

template<typename Iter, typename = void>
struct iterator_traits_internal {};

template<typename Iter>
struct iterator_traits_internal<Iter, void_t<typename Iter::iterator_category,
                                             typename Iter::value_type,
                                             typename Iter::difference_type,
                                             typename Iter::pointer,
                                             typename Iter::reference>> {
  using value_type = typename Iter::value_type;
  using difference_type = typename Iter::difference_type;
  using pointer = typename Iter::pointer;
  using reference = typename Iter::reference;
  using iterator_category = typename Iter::iterator_category;
};

// SFINAE只在函数模板参数推导以及类特化时生效，这里采用继承解决SFINAE无法生效的问题
template<typename Iter>
struct iterator_traits : public iterator_traits_internal<Iter> {};

template<typename T>
struct iterator_traits<T*> {
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  // 迭代器的类型标签
  using iterator_category = random_access_iterator_tag;
};

template<typename T>
struct iterator_traits<const T*> {
  using value_type = const T;
  using difference_type = std::ptrdiff_t;
  using pointer = const T*;
  using reference = const T&;
  using iterator_category = random_access_iterator_tag;
};

} // namespace hstl

#endif  // ITERATOR_HPP_