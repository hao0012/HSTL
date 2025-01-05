#ifndef SHARED_PTR_HPP_
#define SHARED_PTR_HPP_

#include <atomic>
#include <cstddef>
#include <exception>

#include "internal/smart_ptr.hpp"
#include "internal/compressed_pair.hpp"
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

  bool lock() noexcept {
    auto shared = get_shared();
    while (shared > 0) {
      if (shared_count_.compare_exchange_weak(shared, shared + 1,
                                              std::memory_order_relaxed,
                                              std::memory_order_relaxed)) {
        return true;
      }
      shared = get_shared();
    }
    return false;
  }

  virtual void* get_deleter() noexcept = 0;

  virtual void release_object() noexcept = 0;
  virtual void release_this() noexcept = 0;

  virtual ~counter() = default;

  // TODO(hao): 自己实现atomic
  std::atomic<size_t> shared_count_;
  std::atomic<size_t> weak_count_;
};

template <typename T, typename Deleter>
class counter_t : public counter {
 public:
  using value_type = T;
  using deleter_type = Deleter;
  static_assert(is_pointer<T>::value, "counter_t<T>: T must be pointer type");

  counter_t(value_type value, deleter_type deleter)
      : pair_{value, move(deleter)} {}

  // TODO(hao): enable RTTI or not has different behavior
  void* get_deleter() noexcept override { return &pair_.second(); }

  void release_object() noexcept override {
    if (pair_.first() != nullptr) {
      pair_.second()(pair_.first());
      pair_.first() = nullptr;
    }
  }

  void release_this() noexcept override {
    release_object();
    delete this;
  }

 private:
  compressed_pair<value_type, deleter_type> pair_;
};

template <typename T>
class counter_emplace : public counter {
 public:
  using value_type = T;

  template <typename... Args>
  counter_emplace(Args&&... args) : counter(), value_{forward<Args>(args)...} {}

  value_type* get_value_ptr() { return &value_; }

  void* get_deleter() noexcept override { return nullptr; }

  // 这里需要先析构，否则计数不对（引入enable_shared_from_this后会发生计数减不到0的问题）
  void release_object() noexcept override { value_.~value_type(); }
  void release_this() noexcept override { ::operator delete(this); }

 private:
  // 数据和shared_count存放在同一个内存块中
  value_type value_;
};

template <typename T>
class weak_ptr;
template <typename T>
class enable_shared_from_this;

// 专门为shared_ptr<void>类型提供的，保证T == void时，T& operator*()不会编译出错
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

struct bad_weak_ptr : std::exception {
  const char* what() const noexcept override { return "bad_weak_ptr"; }
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
  using default_deleter_type = default_deleter<T>;
  using element_type = T;

  constexpr shared_ptr() noexcept : ptr_{nullptr}, count_{nullptr} {};
  constexpr shared_ptr(
      std::nullptr_t) noexcept  // noexcept要求我们不为count申请内存
      : ptr_{nullptr}, count_{nullptr} {};

  // poscondition: use_count == 1
  template <typename Deleter>
  shared_ptr(std::nullptr_t ptr, Deleter deleter)
      : ptr_{nullptr}, count_{nullptr} {
    try {
      count_ = new counter_t<element_type*, Deleter>(nullptr, deleter);
    } catch (...) { // std::bad_alloc
      deleter(ptr);
      throw;
    }
    do_enable_shared_from_this(*this, ptr);
  };

  template <typename Y>
  shared_ptr(Y* ptr) : ptr_{ptr} {
    try {
      count_ =
        new counter_t<Y*, default_deleter_type>(ptr, default_deleter_type());
    } catch (...) {
      delete ptr;
      throw;
    }
    do_enable_shared_from_this(*this, ptr);
  };

  template <typename Y, typename Deleter>
  shared_ptr(Y* ptr, Deleter deleter) : ptr_{ptr} {
    try {
      count_ = new counter_t<Y*, Deleter>(ptr, deleter);
    } catch (...) {
      deleter(ptr);
      throw;
    }
    do_enable_shared_from_this(*this, ptr);
  };

  shared_ptr(const shared_ptr& other) noexcept
      : ptr_{other.ptr_}, count_{other.count_} {
    if (count_ != nullptr) {
      count_->increase_shared();
    }
  }

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other) noexcept
      : ptr_{other.ptr_}, count_{other.count_} {
    if (count_ != nullptr) {
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
  shared_ptr(const weak_ptr<Y>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    count_->lock();
    if (count_->get_shared() == 0) {
      throw bad_weak_ptr();
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
    // count_不会早于ptr_被析构
    if (count_ == nullptr) {
      return;
    }
    if (count_->decrease_shared() == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      count_->release_object();
      // 只有一个shared_ptr对象会持有weak_count，所以这里只需要减1次，
      // 不需要每个shared_ptr对象都减
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

  template <typename Y, typename Deleter>
  void reset(Y* ptr, Deleter d) {
    shared_ptr(ptr, d).swap(*this);
  }

  void swap(shared_ptr& other) noexcept {
    auto ptr = this->ptr_;
    this->ptr_ = other.ptr_;
    other.ptr_ = ptr;

    auto count = this->count_;
    this->count_ = other.count_;
    other.count_ = count;
  }

  // --------------------------- Observers ------------------------------- //
  element_type* get() { return ptr_; }

  // https://en.cppreference.com/w/cpp/memory/shared_ptr/operator*
  reference_type operator*() { return *ptr_; }
  element_type* operator->() { return ptr_; }

  template <typename U>
  using enable_if_array = enable_if_t<is_array_v<U>>;
  // https://en.cppreference.com/w/cpp/memory/shared_ptr/operator_at
  template <typename U = T, typename = enable_if_array<U>>
  reference_type operator[](std::ptrdiff_t idx) const {
    return ptr_[idx];
  }

  /**
   * @return 拥有object所有权的shared_ptr对象的个数
   *        （注意，这与指针是否为nullptr无关）
   */
  size_t use_count() { return count_ == nullptr ? 0 : count_->get_shared(); }
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
  do_enable_shared_from_this(p, p.ptr_);
  return p;
}

template <typename T, typename U>
void do_enable_shared_from_this(shared_ptr<T> sp,
                                enable_shared_from_this<U>* current) {
  if (current != nullptr) {
    current->weak_this_ = sp;
  }
}

template <typename T>
void do_enable_shared_from_this(shared_ptr<T> ptr, ...) {}

template <typename T>
class weak_ptr {
 public:
  template <typename Y>
  friend class weak_ptr;
  template <typename Y>
  friend class shared_ptr;

  constexpr weak_ptr() noexcept : ptr_{nullptr}, count_{nullptr} {};
  weak_ptr(const weak_ptr& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (count_ != nullptr) {
      count_->increase_weak();
    }
  }

  template <typename Y>
  weak_ptr(const weak_ptr<Y>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (count_ != nullptr) {
      count_->increase_weak();
    }
  }

  // TODO(hao): 为什么没有下面的这个函数？
  weak_ptr(const shared_ptr<T>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (count_ != nullptr) {
      count_->increase_weak();
    }
  }

  template <typename Y>
  weak_ptr(const shared_ptr<Y>& r) noexcept : ptr_{r.ptr_}, count_{r.count_} {
    if (count_ != nullptr) {
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
    if (count_ == nullptr) {
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
    auto ptr = this->ptr_;
    this->ptr_ = r.ptr_;
    r.ptr_ = ptr;

    auto count = this->count_;
    this->count_ = r.count_;
    r.count_ = count;
  }

  size_t use_count() const noexcept {
    return ptr_ == nullptr ? 0 : count_->get_shared();
  }

  bool expired() const noexcept { return use_count() == 0; }

  shared_ptr<T> lock() const noexcept {
    shared_ptr<T> p;
    if (count_->lock()) {
      p.ptr_ = ptr_;
      p.count_ = count_;
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