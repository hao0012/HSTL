#ifndef SMART_PTR_HPP_
#define SMART_PTR_HPP_

#include <type_traits>
namespace hstl {

template <typename T>
struct default_deleter {
  constexpr default_deleter() = default;

  /**
   * 在shared_ptr拷贝时使用，需要将deleter也拷贝过来。
   * 对于default_deleter来说，因为属于同一个继承体系，
   * 析构时delete会正确调用对应的析构函数，因此不需要从
   * 删除器中取得什么东西。
   *
   * 对于自定义的删除器：
   * 1. 拷贝构造中，count_被整个拷贝，删除器会在这个过程中被拷贝
   * 2. 赋值运算符中，使用copy and swap，直接获得新的删除器
   *    可以保证正确的删除器被调用
   */
  template <typename U,
            typename std::enable_if_t<std::is_convertible<U*, T*>::value>>
  default_deleter(const default_deleter<U>&) noexcept {}

  void operator()(T* ptr) {
    // TODO(hao): is_complete_type
    // static_assert(hstl::is_complete_type<T>::value, "Attempting to call the
    // destructor of an incomplete type");
    delete ptr;
  }
};

template <typename T>
struct default_deleter<T[]> {
  constexpr default_deleter() = default;
  template<typename U /* TODO(hao), typename std::is_array_cv_convertible<U*, T*>::value */>
  default_deleter(const default_deleter<U[]>&) noexcept {}

  void operator()(T* ptr) { delete[] ptr; }
};

}  // namespace hstl

#endif  // SMART_PTR_HPP_