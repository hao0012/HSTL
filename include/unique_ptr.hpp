#ifndef UNIQUE_PTR_HPP_
#define UNIQUE_PTR_HPP_

// #include "internal/smart_ptr.hpp"
#include "utility.hpp"

namespace hstl {

template <typename T>
class unique_ptr {
 public:
  template <typename Y>
  friend class unique_ptr;

  using element_type = T;
  using pointer = element_type*;

  constexpr unique_ptr() noexcept : ptr_{nullptr} {}
  constexpr unique_ptr(std::nullptr_t) noexcept : ptr_{nullptr} {}
  explicit unique_ptr(pointer p) noexcept : ptr_{p} {}

  unique_ptr(const unique_ptr& u) = delete;

  unique_ptr(unique_ptr&& u) noexcept : ptr_{u.ptr_} { u.ptr_ = nullptr; }
  template <typename Y>
  unique_ptr(unique_ptr<Y>&& u) noexcept : ptr_{u.ptr_} {
    u.ptr_ = nullptr;
  }

  unique_ptr& operator=(const unique_ptr& u) = delete;

  unique_ptr& operator=(unique_ptr&& r) noexcept {
    unique_ptr(hstl::move(r)).swap(*this);
    return *this;
  }

  template <typename Y>
  unique_ptr& operator=(unique_ptr<Y>&& r) noexcept {
    unique_ptr(hstl::move(r)).swap(*this);
    return *this;
  }

  unique_ptr& operator=(std::nullptr_t) noexcept {
    unique_ptr().swap(*this);
    return *this;
  }

  ~unique_ptr() { delete ptr_; }

  pointer get() const noexcept { return ptr_; }

  operator bool() const noexcept { return ptr_ != nullptr; }

  void reset(pointer p = pointer()) noexcept { unique_ptr(p).swap(*this); }

  void swap(unique_ptr& other) noexcept {
    auto p = ptr_;
    ptr_ = other.ptr_;
    other.ptr_ = p;
  }

  pointer release() noexcept {
    auto p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  hstl::add_lvalue_reference_t<T> operator*() const
      noexcept(noexcept(*hstl::declval<pointer>())) {
    return *ptr_;
  }

  pointer operator->() const noexcept { return ptr_; }

 private:
  pointer ptr_;
};

// http://stackoverflow.com/questions/12580432/why-does-c11-have-make-shared-but-not-make-unique
// http://herbsutter.com/2013/05/29/gotw-89-solution-smart-pointers/
template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(hstl::forward<Args>(args)...));
}

}  // namespace hstl

#endif  // UNIQUE_PTR_HPP_