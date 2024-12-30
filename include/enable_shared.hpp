#ifndef ENABLE_SHARED_HPP_
#define ENABLE_SHARED_HPP_

#include "shared_ptr.hpp"

namespace hstl {

/**
 * enable_shared_from_this可以从对象得到对应的shared_ptr<T>，
 * 防止多个计数器管理一个对象（当使用shared_ptr(&obj)时，且obj已经被其他
 * shared_ptr管理，就会出现多个计数器管理同一个对象的情况）
 */
template <class T>
class enable_shared_from_this {
 public:
  template <typename U, typename V>
  friend void do_enable_shared_from_this(shared_ptr<U> ptr,
                                         enable_shared_from_this<V>* current);

  shared_ptr<T> shared_from_this() { return shared_ptr<T>(weak_this_); }
  shared_ptr<const T> shared_from_this() const {
    return shared_ptr<const T>(weak_this_);
  }
  weak_ptr<T> weak_from_this() noexcept { return weak_this_; }
  weak_ptr<const T> weak_from_this() const noexcept { return weak_this_; }

 protected:
  constexpr enable_shared_from_this() noexcept = default;
  enable_shared_from_this(const enable_shared_from_this& other) noexcept =
      default;
  enable_shared_from_this& operator=(
      const enable_shared_from_this& other) noexcept = default;
  ~enable_shared_from_this() = default;

 private:
  weak_ptr<T> weak_this_;
};

}  // namespace hstl

#endif  // ENABLE_SHARED_HPP_