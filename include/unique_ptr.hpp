#ifndef UNIQUE_PTR_HPP_
#define UNIQUE_PTR_HPP_

#include "internal/compressed_pair.hpp"
#include "internal/smart_ptr.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace hstl {

template <typename T, typename Deleter = default_deleter<T>>
class unique_ptr {
 public:
  using element_type = T;
  using pointer = element_type*;
  using deleter_type = Deleter;

  constexpr unique_ptr() noexcept : pair_{nullptr} {}
  constexpr unique_ptr(std::nullptr_t) noexcept : pair_{nullptr} {}
  explicit unique_ptr(pointer p) noexcept : pair_{p} {}

  // deleter_type为引用
  unique_ptr(pointer p,
             conditional_t<is_lvalue_reference_v<deleter_type>, deleter_type,
                           add_lvalue_reference_t<deleter_type>>
                 d) noexcept
      : pair_{p, d} {}
  // deleter_type为右值
  unique_ptr(pointer p, remove_reference_t<deleter_type>&& d) noexcept
      : pair_{p, move(d)} {}

  unique_ptr(const unique_ptr& u) = delete;

  unique_ptr(unique_ptr&& u) noexcept
      : pair_{u.release(), forward<deleter_type>(u.get_deleter())} {}

  template <typename U, typename E>
  unique_ptr(unique_ptr<U, E>&& u) noexcept
      : pair_(u.release(), forward<E>(u.get_deleter())) {}

  unique_ptr& operator=(const unique_ptr& u) = delete;

  unique_ptr& operator=(unique_ptr&& r) noexcept {
    unique_ptr(hstl::move(r)).swap(*this);
    return *this;
  }

  template <typename U, typename E>
  unique_ptr& operator=(unique_ptr<U, E>&& r) noexcept {
    unique_ptr(hstl::move(r)).swap(*this);
    return *this;
  }

  unique_ptr& operator=(std::nullptr_t) noexcept {
    unique_ptr().swap(*this);
    return *this;
  }

  ~unique_ptr() { pair_.second()(pair_.first()); }

  pointer get() const noexcept { return pair_.first(); }

  Deleter& get_deleter() noexcept { return pair_.second(); }
  const Deleter& get_deleter() const noexcept { return pair_.second(); }

  operator bool() const noexcept { return pair_.first() != nullptr; }

  void reset(pointer p = pointer()) noexcept { unique_ptr(p).swap(*this); }

  void swap(unique_ptr& other) noexcept {
    auto p = pair_;
    pair_ = other.pair_;
    other.pair_ = p;
  }

  pointer release() noexcept {
    auto p = pair_.first();
    pair_.first() = nullptr;
    return p;
  }

  add_lvalue_reference_t<T> operator*() const
      noexcept(noexcept(*hstl::declval<pointer>())) {
    return *(pair_.first());
  }

  pointer operator->() const noexcept { return pair_.first(); }

 private:
  compressed_pair<pointer, deleter_type> pair_;
};

// http://stackoverflow.com/questions/12580432/why-does-c11-have-make-shared-but-not-make-unique
// http://herbsutter.com/2013/05/29/gotw-89-solution-smart-pointers/
template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(hstl::forward<Args>(args)...));
}

}  // namespace hstl

#endif  // UNIQUE_PTR_HPP_