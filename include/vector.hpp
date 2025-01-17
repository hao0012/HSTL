#ifndef VECTOR_HPP_
#define VECTOR_HPP_

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <memory>
#include <stdexcept>

#include "internal/compressed_pair.hpp"
#include "type_traits.hpp"
#include "utility.hpp"
#include "memory.hpp"

namespace hstl {

template <typename T, typename Allocator>
class VectorBase {
  using size_type = size_t;
  using allocator_type = Allocator;
  public:
  VectorBase() noexcept(noexcept(allocator_type()))
  : begin_(nullptr), end_(nullptr), capacity_(nullptr, allocator_type()) {}
  VectorBase(const allocator_type& allocator)
  : begin_(nullptr), end_(nullptr), capacity_(nullptr, allocator) {}
	VectorBase(size_type n, const allocator_type& allocator)
  : capacity_(allocator) {
    begin_ = capacity_.second().allocate(n);
    end_ = begin_;
    capacity_.first() = begin_ + n;
  }
  ~VectorBase() {
    if (begin_) {
      capacity_.second().deallocate(begin_, capacity_.first() - begin_);
    }
    begin_ = nullptr;
    end_ = nullptr;
    capacity_.first() = nullptr;
  }

  // npos表示一个无效的位置，当查找失败时返回
  static constexpr size_type npos = static_cast<size_type>(-1);
  // kMaxSize表示vector的最大容量，要比npos小1
  static constexpr size_type kMaxSize = static_cast<size_type>(-2); 

  protected:
  T* begin_;
  T* end_;
  compressed_pair<T*, allocator_type> capacity_;
};

// T: 要求是完整类型且Erasable (C++ 11 - 17)
template <typename T, typename Allocator = std::allocator<T>>
class vector: public VectorBase<T, Allocator> {
  static_assert(hstl::is_same_v<T, typename Allocator::value_type>,
                "Allocator::value_type must be same as T");
  template <typename U, typename Allocator2>
  friend void swap(vector<U, Allocator2>& lhs, vector<U, Allocator2>& rhs);
  
  using base_type = VectorBase<T, Allocator>;
  using base_type::begin_;
  using base_type::end_;
  using base_type::capacity_;

 public:
  using size_type = size_t;
  using allocator_type = Allocator;
  using difference_type = ptrdiff_t;
  using value_type = T;
  using reference       = value_type&;
  using const_reference = const value_type&;
  // TODO allocator_traits的优点是什么？相比于Allocator::pointer
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;

  vector(): base_type() {}
  explicit vector(const Allocator& alloc);
  explicit vector(size_type size, const Allocator& alloc = Allocator());
  vector(size_type size, const value_type& value, const Allocator& alloc = Allocator());
  // https://stackoverflow.com/questions/43821689/c-vector-constructor-instantiation-conflicts
  template<typename InputIt, typename = typename iterator_traits<InputIt>::value_type>
  vector(InputIt first, InputIt last, const Allocator& alloc = Allocator());
  explicit vector(const vector& other);
  vector(vector&& other);
  vector(std::initializer_list<value_type> l, const Allocator& alloc = Allocator());
  ~vector();

  vector& operator=(const vector& other);

  void assign(size_type count, const T& value);
  template<typename InputIt>
  void assign(InputIt first, InputIt last);
  void assign(std::initializer_list<value_type> l);

  allocator_type get_allocator() const { return capacity_.second(); }

  /* ------------- Element access ------------- */
  reference at( size_type pos ) {
    return const_cast<reference>(static_cast<const vector*>(this)->at(pos));
  }
  const_reference at( size_type pos ) const {
    if (begin_ + pos >= end_) {
      throw std::out_of_range("vector::at");
    }
    return *(begin_ + pos);
  }

  reference operator[]( size_type pos ) { return *(begin_ + pos); }
  const_reference operator[]( size_type pos ) const { return *(begin_ + pos); }

  reference front() { return *begin_; }
  const_reference front() const { return *begin_; }
  
  reference back() { return *(begin_ + size() - 1); }
  const_reference back() const { return *(begin_ + size() - 1); }
  
  pointer data() { return begin_; }
  const_pointer data() const { return begin_; }
  /* ------------- Element access ------------- */

  /* ------------- Iterators ------------- */
  iterator begin() { return begin_; }
  const_iterator begin() const { return begin_; }
  const_iterator cbegin() const noexcept { return begin_; }

  iterator end() noexcept { return end_; }
  const_iterator end() const noexcept { return end_; }
  const_iterator cend() const noexcept { return end_; }
  /* ------------- Iterators ------------- */

  /* ------------- Capacity ------------- */
  bool empty() const { return end_ == begin_; }
  size_type size() const { return end_ - begin_; }
  size_type max_size() const { return base_type::kMaxSize; }
  void reserve( size_type new_cap );
  size_type capacity() const { return capacity_.first() - begin_; }
  void shrink_to_fit();
  /* ------------- Capacity ------------- */

  /* ------------- Modifiers ------------- */
  void clear() { erase(begin_, end_); }

  // 与push_back相比多了CopyAssignable: 插入时需要移动后面的元素，会反向地执行赋值操作
  iterator insert( const_iterator pos, const T& value );
  iterator insert( const_iterator pos, T&& value );
  iterator insert( const_iterator pos, size_type count, const T& value );
  // TODO
  template< class InputIt >
  iterator insert( const_iterator pos, InputIt first, InputIt last );
  iterator insert( const_iterator pos, std::initializer_list<T> ilist );

  template<typename... Args>
  iterator emplace(const_iterator pos, Args&&... args);

  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);

  // T: CopyInsertable
  void push_back( const T& value );
  // T: MoveInsertable
  void push_back( T&& value );
  template <typename... Args>
  void emplace_back(Args&&... args);

  void pop_back();
  void resize(size_type count, const value_type& value = value_type());
  void swap(vector& other);
  /* ------------- Modifiers ------------- */
private:
  size_type get_new_capacity(size_type current_capacity) {
    return current_capacity == 0 ? 1 : current_capacity * 2;
  }

  iterator do_insert_range(iterator position, size_type n, const value_type& value);
  template<typename... Args>
  iterator do_insert(iterator position, Args&&... args);
  template<typename... Args>
  iterator do_insert_back(Args&&... args);
};

template <typename T, typename Allocator>
vector<T, Allocator>::vector(const Allocator& alloc)
: VectorBase<T, Allocator>(alloc) {}

template <typename T, typename Allocator>
vector<T, Allocator>::vector(const vector& other)
: VectorBase<T, Allocator>(other.size(), other.capacity_.second()) {
  uninitialized_copy(other.begin_, other.end_, begin_);
  end_ += other.size();
}

template <typename T, typename Allocator>
vector<T, Allocator>::vector(size_type size, const Allocator& alloc)
: VectorBase<T, Allocator>(size, alloc) {
  uninitialized_fill_n(begin_, size, value_type());
  end_ += size;
}

template <typename T, typename Allocator>
vector<T, Allocator>::vector(size_type size, const value_type& value, const Allocator& alloc)
: VectorBase<T, Allocator>(size, alloc) {
  uninitialized_fill_n(begin_, size, value);
  end_ += size;
}

template <typename T, typename Allocator>
template<typename InputIt, typename>
vector<T, Allocator>::vector(InputIt first, InputIt last, const Allocator& alloc)
: VectorBase<T, Allocator>(last - first, alloc) {
  uninitialized_copy(first, last, begin_);
  end_ += last - first;
}

template <typename T, typename Allocator>
vector<T, Allocator>::vector(vector&& other)
: VectorBase<T, Allocator>(move(other.capacity_.second())) {
  begin_ = other.begin_;
  end_ = other.end_;
  capacity_.first() = other.capacity_.first();

  other.begin_ = nullptr;
  other.end_ = nullptr;
  other.capacity_.first() = nullptr;
}

template <typename T, typename Allocator>
vector<T, Allocator>::vector(std::initializer_list<value_type> l, const Allocator& alloc)
  : vector(l.begin(), l.end(), alloc) {}

// TODO(hao): is_trivially_destructible
template <typename T, typename Allocator>
vector<T, Allocator>::~vector() {
  destroy(begin_, end_);
};

template <typename T, typename Allocator>
vector<T, Allocator>& vector<T, Allocator>::operator=(const vector& other) {
  vector(other).swap(*this);
  return *this;
}

template <typename T, typename Allocator>
void vector<T, Allocator>::assign(size_type count, const T& value) {
  // 1. count比当前容量大，需要重新分配内存
  if (count > capacity_.first() - begin_) {
    vector(count, value, capacity_.second()).swap(*this);
  } 
  // 2. count比当前size大，需要填充value
  else if (count > size()) {
    std::fill(begin_, end_, value);
    uninitialized_fill_n(end_, count - size(), value);
  } 
  // 3. count比当前size小，需要析构多余的元素
  else {
    std::fill(begin_, begin_ + count, value);
    erase(begin_ + count, end_);
  }
}
// TODO(hao): move
// reserve() cannot be used to reduce the capacity of 
// the container; to that end shrink_to_fit() is provided.
template <typename T, typename Allocator>
void vector<T, Allocator>::reserve( size_type new_cap ) {
  if (new_cap <= capacity_.first() - begin_) {
    return;
  }
  auto mem = capacity_.second().allocate(new_cap);
  uninitialized_move(begin_, end_, mem);
  auto s = size();
  
  destroy(begin_, end_);
  capacity_.second().deallocate(begin_, capacity_.first() - begin_);

  begin_ = mem;
  end_ = begin_ + s;
  capacity_.first() = begin_ + new_cap;
}

// TODO(hao): move_iterator
template <typename T, typename Allocator>
void vector<T, Allocator>::shrink_to_fit() {
  vector(begin_, end_, capacity_.second()).swap(*this);
}

template <typename T, typename Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::insert( const_iterator pos, const T& value ) {
  return do_insert(const_cast<iterator>(pos), value);
}

// TODO(hao): move_iterator
template <typename T, typename Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::insert( const_iterator pos, T&& value ) {
  return do_insert(const_cast<iterator>(pos), forward<T>(value));
}

template <typename T, typename Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::insert( const_iterator pos, size_type count, const T& value ) {
  return do_insert_range(const_cast<iterator>(pos), count, value);
}

template <typename T, typename Allocator>
template<typename... Args>
typename vector<T, Allocator>::iterator vector<T, Allocator>::emplace(const_iterator pos, Args&&... args) {
  return do_insert(const_cast<iterator>(pos), forward<Args>(args)...);
}

template<typename T, typename Allocator>
template<typename... Args>
typename vector<T, Allocator>::iterator vector<T, Allocator>::do_insert(iterator pos, Args&&... args) {
  if (size() < capacity()) {
    // move_backward采用赋值运算符，move到的位置上必须已经有对象
    ::new (static_cast<void*>(end_)) value_type(move(*(end_ - 1)));
    std::move_backward(pos, end_ - 1, end_);
    ++end_;
    // 方法1：需要1次构造以及1次析构，但该方法可能无法保证异常安全
    *pos = move(value_type(forward<Args>(args)...));
    // 方法2：需要2次构造以及一次析构
    value_type tmp(forward<Args>(args)...);
    destroy_at(pos);
    ::new (static_cast<void*>(pos)) value_type(move(tmp));

    // 错误方法：需要类有swap函数
    // value_type(forward<Args>(args)...).swap(*pos);
    return pos;
  }
  auto new_cap = get_new_capacity(capacity());
  auto mem = capacity_.second().allocate(new_cap);
  uninitialized_move(begin_, pos, mem);
  ::new (static_cast<void*>(mem + (pos - begin_))) value_type(forward<Args>(args)...);
  uninitialized_move(pos, end_, mem + (pos - begin_) + 1);

  auto offset = pos - begin_;
  auto s = size();

  destroy(begin_, end_);
  capacity_.second().deallocate(begin_, capacity_.first() - begin_);
  
  begin_ = mem;
  end_ = begin_ + s + 1;
  capacity_.first() = begin_ + new_cap;

  return begin_ + offset;
}

template<typename T, typename Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::do_insert_range(iterator position, size_type n, const value_type& value) {
  if (size() + n <= capacity()) {
    if (n <= end_ - position) {
      uninitialized_move(end_ - n, end_, end_);
      std::move_backward(position, end_ - n, end_);
      uninitialized_fill_n(position, n, value);
    } else {
      uninitialized_move(position, end_, end_ + n);
      destroy(position, end_);
      uninitialized_fill_n(position, n, value);
    }
    return position;
  }
  auto new_cap = get_new_capacity(capacity());
  new_cap = new_cap < size() + n ? size() + n : new_cap;
  auto mem = capacity_.second().allocate(new_cap);

  uninitialized_move(begin_, position, mem);
  uninitialized_fill_n(mem + (position - begin_), n, value);
  uninitialized_move(position, end_, mem + (position - begin_) + n);

  auto offset = position - begin_;
  auto s = size();
  
  destroy(begin_, end_);
  capacity_.second().deallocate(begin_, capacity_.first() - begin_);

  begin_ = mem;
  end_ = begin_ + s + n;
  capacity_.first() = begin_ + new_cap;
  
  return begin_ + offset;
}

template<typename T, typename Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::erase(const_iterator pos) {
  return erase(pos, pos + 1);
}

template<typename T, typename Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::erase(const_iterator first, const_iterator last) {
  assert(first >= begin_ && last <= end_);
  auto it1 = first;
  for (auto it2 = last; it2 != end_; ++it1, ++it2) {
    *it1 = *it2;
  }
  destory(it1, end_);
  end_ = it1;
}
template<typename T, typename Allocator>
void vector<T, Allocator>::push_back( const T& value ) {
  do_insert_back(value);
}

template<typename T, typename Allocator>
void vector<T, Allocator>::push_back( T&& value ) {
  do_insert_back(forward<T>(value));
}

template<typename T, typename Allocator>
template <typename... Args>
void vector<T, Allocator>::emplace_back(Args&&... args) {
  do_insert_back(forward<Args>(args)...);
}

template<typename T, typename Allocator>
template<typename... Args>
typename vector<T, Allocator>::iterator vector<T, Allocator>::do_insert_back(Args&&... args) {
  if (size() < capacity()) {
    ::new (static_cast<void*>(end_)) T(forward<Args>(args)...);
    ++end_;
  } else {
    auto new_cap = get_new_capacity(capacity());
    auto mem = capacity_.second().allocate(new_cap);
    uninitialized_move(begin_, end_, mem);
    auto s = size();
    ::new (static_cast<void*>(mem + s)) value_type(forward<Args>(args)...);
    destroy(begin_, end_);
    capacity_.second().deallocate(begin_, capacity_.first() - begin_);
    begin_ = mem;
    end_ = begin_ + s + 1;
    capacity_.first() = begin_ + new_cap;
  }
  return end_ - 1;
}

template<typename T, typename Allocator>
void vector<T, Allocator>::pop_back() {
  destroy_at(--end_);
}

template<typename T, typename Allocator>
void vector<T, Allocator>::resize(size_type count, const value_type& value) {
  if (count < size()) {
    destroy(begin_ + count, end_);
  } else if (count > size()) {
    if (count > capacity()) {
      auto new_cap = get_new_capacity(capacity());
      auto mem = capacity_.second().allocate(new_cap);
      uninitialized_move(begin_, end_, mem);
      uninitialized_fill_n(mem + size(), count - size(), value);
      destory(begin_, end_);
      begin_ = mem;
      end_ = begin_ + count;
      capacity_.first() = begin_ + new_cap;
    } else {
      uninitialized_fill_n(end_, count - size(), value);
      end_ = begin_ + count;
    }
  }
}

template<typename T, typename Allocator>
void vector<T, Allocator>::swap(vector& other) {
  std::swap(begin_, other.begin_);
  std::swap(end_, other.end_);
  std::swap(capacity_, other.capacity_);
}

template<typename T, typename Allocator>
void swap(vector<T, Allocator>& lhs, vector<T, Allocator>& rhs) {
  lhs.swap(rhs);
}

}  // namespace hstl

#endif  // VECTOR_HPP_