#ifndef SHARED_PTR_HPP_
#define SHARED_PTR_HPP_

#include <atomic>
#include <cstddef>

#include "utility.hpp"

namespace hstl {

struct counter {
  counter(size_t shared_count = 1, size_t weak_count = 1)
      : shared_count_{shared_count}, weak_count_(weak_count) {}

  void increase_shared() {
    shared_count_.fetch_add(1, std::memory_order_relaxed);
  }
  void increase_weak() { weak_count_.fetch_add(1, std::memory_order_relaxed); }

  size_t decrease_shared() {
    return shared_count_.fetch_sub(1, std::memory_order_release);
  }
  size_t decrease_weak() {
    return weak_count_.fetch_sub(1, std::memory_order_release);
  }

  size_t get_shared() { return shared_count_.load(std::memory_order_relaxed); }
  size_t get_weak() { return weak_count_.load(std::memory_order_relaxed); }

  virtual void release_object() noexcept = 0;
  virtual void release_this() noexcept = 0;

  // TODO(hao): 自己实现atomic
  std::atomic<size_t> shared_count_;
  std::atomic<size_t> weak_count_;
};

template <typename T>
class counter_t : public counter {
 public:
  using value_type = T;
  static_assert(is_pointer<T>::value, "counter_t<T>: T must be pointer type");

  counter_t(value_type value) : counter(), value_{value} {}

  void release_object() noexcept override {
    if (value_ != nullptr) {
      delete value_;
      value_ = nullptr;
    }
  }

  void release_this() noexcept override {
    release_object();
    delete this;
  }

 private:
  value_type value_;
};

template <typename T>
class counter_emplace : public counter {
 public:
  using value_type = T;

  template <typename... Args>
  counter_emplace(Args&&... args) : counter(), value_{forward<Args>(args)...} {}

  value_type* get_value_ptr() { return &value_; }

  // 一起释放
  void release_object() noexcept override {}
  void release_this() noexcept override { delete this; }

 private:
  // 数据和shared_count存放在同一个内存块中
  value_type value_;
};

template <typename T>
class weak_ptr;

/**
 * 专门为shared_ptr<void>类型提供的，保证T == void时，T& operator*()不会编译出错
 */
template <typename T>
struct shared_ptr_traits {
  using reference_type = T&;
};

template <>
struct shared_ptr_traits<void> {
  using reference_type = void;
};

template <>
struct shared_ptr_traits<void const> {
  using reference_type = void;
};

template <>
struct shared_ptr_traits<void volatile> {
  using reference_type = void;
};

template <>
struct shared_ptr_traits<void const volatile> {
  using reference_type = void;
};

template <typename T>
class shared_ptr {
  template <typename U>
  friend class shared_ptr;
  template <typename U>
  friend class weak_ptr;
  template <typename U, typename... Args>
  friend shared_ptr<U> make_shared(Args&&... args);

 public:
  using reference_type = typename shared_ptr_traits<T>::reference_type;
  using element_type = T;

  constexpr shared_ptr() noexcept : ptr_{nullptr}, count_{nullptr} {};
  constexpr shared_ptr(std::nullptr_t) noexcept
      : ptr_{nullptr}, count_{nullptr} {};

  /**
   * 1.
   * ptr必须是还未被管理的指针，不能已经被其他智能指针管理，否则会导致多个counter管理一个对象
   * 当其中一个的shared_count_为0时会释放资源，但是它并不知道其他count还持有该资源。
   * 2. SFINE确保Y是T或T的派生类(ptr_{ptr})
   */
  template <typename Y>
  shared_ptr(Y* ptr) : ptr_{ptr} {
    if (ptr != nullptr) {
      count_ = new counter_t<Y*>(ptr);
    }
  };

  shared_ptr(const shared_ptr& other) noexcept
      : ptr_{other.ptr_}, count_{other.count_} {
    if (ptr_ != nullptr) {
      count_->increase_shared();
    }
  }

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other) noexcept
      : ptr_{other.ptr_}, count_{other.count_} {
    if (ptr_ != nullptr) {
      count_->increase_shared();
    }
  }

  shared_ptr(shared_ptr&& other) noexcept
      : ptr_{other.ptr_}, count_{other.count_} {
    other.ptr_ = nullptr;
    other.count_ = nullptr;
  }

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other) noexcept
      : ptr_{other.ptr_}, count_{other.count_} {
    other.ptr_ = nullptr;
    other.count_ = nullptr;
  }

  template <typename Y>
  shared_ptr(const weak_ptr<Y>& r) noexcept
      : ptr_{r.ptr_}, count_{r.count_} {
    if (ptr_ != nullptr) {
      count_->increase_shared();
    }
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    // other在shared_ptr(other)拷贝构造函数执行时成员被置为nullptr，不需要再额外操作
    shared_ptr(other).swap(*this);
    return *this;
  }

  template <typename Y>
  shared_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    shared_ptr(other).swap(*this);
    return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    shared_ptr(std::move(other)).swap(*this);
    return *this;
  }

  template <typename Y>
  shared_ptr& operator=(shared_ptr<Y>&& other) noexcept {
    shared_ptr(std::move(other)).swap(*this);
    return *this;
  }

  ~shared_ptr() {
    if (ptr_ == nullptr) {
      return;
    }
    if (count_->decrease_shared() == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      count_->release_object();
      if (count_->decrease_weak() == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        count_->release_this();
      }
    }
    ptr_ = nullptr;
    count_ = nullptr;
  }

  void reset() noexcept { shared_ptr().swap(*this); }

  template <typename Y>
  void reset(Y* ptr) noexcept {
    shared_ptr(ptr).swap(*this);
  }

  void swap(shared_ptr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(count_, other.count_);
  }

  // --------------------------- Observers ------------------------------- //
  element_type* get() { return ptr_; }

  // https://en.cppreference.com/w/cpp/memory/shared_ptr/operator*
  reference_type operator*() { return *ptr_; }
  element_type* operator->() { return ptr_; }

  template <typename U>
  using enable_if_array = enable_if_t<is_array<U>::value>;
  // https://en.cppreference.com/w/cpp/memory/shared_ptr/operator_at
  template <typename U = T, typename = enable_if_array<U>>
  reference_type operator[](std::ptrdiff_t idx) const {
    return (*ptr_)[idx];
  }

  size_t use_count() { return ptr_ == nullptr ? 0 : count_->get_shared(); }
  operator bool() { return ptr_ != nullptr; }

 private:
  element_type* ptr_;
  counter* count_;
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  shared_ptr<T> p;
  auto count = new counter_emplace<T>(forward<Args>(args)...);
  p.ptr_ = count->get_value_ptr();
  p.count_ = count;
  return p;
}

template <typename T>
class weak_ptr {
 public:
  template <typename Y>
  friend class weak_ptr;
  template <typename Y>
  friend class shared_ptr;

  constexpr weak_ptr() noexcept : ptr_{nullptr}, count_{nullptr} {};
  weak_ptr(const weak_ptr& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (ptr_ != nullptr) {
      count_->increase_weak();
    }
  }

  template <typename Y>
  weak_ptr(const weak_ptr<Y>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (ptr_ != nullptr) {
      count_->increase_weak();
    }
  }

  // TODO(hao): 为什么没有下面的这个函数？
  weak_ptr(const shared_ptr<T>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (ptr_ != nullptr) {
      count_->increase_weak();
    }
  }

  template <typename Y>
  weak_ptr(const shared_ptr<Y>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (ptr_ != nullptr) {
      count_->increase_weak();
    }
  }

  weak_ptr(weak_ptr&& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    r.ptr_ = nullptr;
    r.count_ = nullptr;
  }

  template <typename Y>
  weak_ptr(weak_ptr<Y>&& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    r.ptr_ = nullptr;
    r.count_ = nullptr;
  }

  weak_ptr& operator=(const weak_ptr& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }

  template <typename Y>
  weak_ptr& operator=(const weak_ptr<Y>& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }

  template <typename Y>
  weak_ptr& operator=(const shared_ptr<Y>& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }

  weak_ptr& operator=(weak_ptr&& r) noexcept {
    weak_ptr(std::move(r)).swap(*this);
    return *this;
  }

  template <typename Y>
  weak_ptr& operator=(weak_ptr<Y>&& r) noexcept {
    weak_ptr(std::move(r)).swap(*this);
    return *this;
  }

  ~weak_ptr() {
    if (ptr_ == nullptr) {
      return;
    }
    if (count_->decrease_weak() == 1 /* && count_->get_shared() == 0 */) {
      std::atomic_thread_fence(std::memory_order_acquire);
      count_->release_this();
    }
    ptr_ = nullptr;
    count_ = nullptr;
  }

  void reset() noexcept { weak_ptr().swap(*this); }

  void swap(weak_ptr& r) noexcept {
    std::swap(ptr_, r.ptr_);
    std::swap(count_, r.count_);
  }

  size_t use_count() const noexcept {
    return ptr_ == nullptr ? 0 : count_->get_shared();
  }

  bool expired() const noexcept { return use_count() == 0; }

  shared_ptr<T> lock() const noexcept {
    shared_ptr<T> p;

    auto shared = use_count();
    while (shared > 0) {
      if (count_->shared_count_.compare_exchange_weak(
              shared, shared + 1, std::memory_order_relaxed,
              std::memory_order_relaxed)) {
        p.ptr_ = ptr_;
        p.count_ = count_;
        break;
      }
      shared = use_count();
    }
    return p;
  }

 private:
  using element_type = T;
  element_type* ptr_;
  counter* count_;
};

}  // namespace hstl

#endif  // SHARED_PTR_HPP_