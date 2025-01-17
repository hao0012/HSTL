#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <memory>
#include "iterator.hpp"

namespace hstl {

// TODO(hao): optimize using memmove exception safety
// [first, last]范围内的对象拷贝到d_first开始的空间
template<typename InputIt, typename NoThrowForwardIt>
NoThrowForwardIt uninitialized_copy(InputIt first, InputIt last, NoThrowForwardIt d_first) {
  using T = typename iterator_traits<NoThrowForwardIt>::value_type;
  for (; first != last; ++d_first, (void) ++first) {
    ::new (static_cast<void*>(std::addressof(*d_first))) T(*first);
  }
  return d_first;
}

// 将[first, last)范围内的对象移动到d_first开始的空间
template< typename InputIt, typename NoThrowForwardIt >
NoThrowForwardIt uninitialized_move( InputIt first, InputIt last, NoThrowForwardIt d_first ) {
  using T = typename iterator_traits<NoThrowForwardIt>::value_type;
  for (; first != last; ++d_first, (void) ++first) {
    // 优先使用移动构造函数， 如果移动构造函数不存在则使用拷贝构造函数
    ::new (static_cast<void*>(std::addressof(*d_first))) T(move(*first));
  }
  return d_first;
}

// TODO(hao): optimize
// [first, first + count)范围内统一拷贝构造对象value
template<typename ForwardIt, typename Size, typename T>
ForwardIt uninitialized_fill_n(ForwardIt first, Size count, const T& value) {
  using U = typename iterator_traits<ForwardIt>::value_type;
  for (; count > 0; --count, (void) ++first) {
    ::new (static_cast<void*>(std::addressof(*first))) U(value);
  }
  return first;
}

template< typename T >
void destroy_at( T* p ) {
  p->~T();
}

template< typename ForwardIt >
void destroy( ForwardIt first, ForwardIt last ) {
  for (; first != last; ++first) {
    destroy_at(std::addressof(*first));
  }
}

} // namespace hstl

#endif  // MEMORY_HPP_