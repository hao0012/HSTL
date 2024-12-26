# 智能指针

## shared_ptr

### 计数器

#### 体系

counter是基类，有2个子类：

1. counter_t：使用`shared_ptr<T>(new T(...))`创建指针时使用，T和counter_t在内存中分离存放。
2. counter_emplace：使用`make_shared<T>(...)`创建指针时使用，T就在counter_emplace中，使用一块连续的内存。

#### 初始化计数

1. `shared_count_`初始为1，因为刚创建时肯定有一个shared_ptr指向它。
2. `weak_count_`也初始为1，是为了防止shared_ptr和weak_ptr同时析构并同时调用counter的析构函数(double free)或都不调用(memory leak)。

https://stackoverflow.com/questions/43297517/stdshared-ptr-internals-weak-count-more-than-expected

weak_ptr的出现使得counter_t和counter_emplace在对象释放的时机上产生不同（对象和counter的生命周期不再一致）：

1. counter_t: 对象T和counter的内存分开管理，`shared_count == 0`时就可以释放对象了，减少内存占用，在两个计数都减为0时才释放counter_t自身。
2. counter_emplace: 由于对象就在counter_emplace中，因此二者需要同时释放，不能提前释放对象T。

#### 修改计数

以shared_count_为例：

```c++
shared_count_.fetch_add(1, std::memory_order_relaxed);
shared_count_.fetch_sub(1, std::memory_order_release);
```

1. 增加计数采用relaxed次序，因为之后不会立即对资源进行释放，无需同步。
2. 减少计数采用release次序，让对象T上的操作在减少计数之前发生，之后可能释放对象，如果采用relaxed次序，op_a可能重排序到decr后执行，导致op_a实际在~value_type()之后执行：

```c++
//          A       |        B
//     decr(to 1)   |
//                  |      t.op_b
//                  |    decr(to 0)
//                  |   ~value_type()
//       t.op_a     |
```

### 构造函数

以下说明一些需要考虑到的点：

#### `shared_ptr<T>(new ptr(Args))`

ptr必须是还未被管理的指针，多个智能指针管理一个对象，会发生double free或访问已释放资源的问题：

```c++
template<typename Y>
explicit shared_ptr(Y* ptr) : ptr_{ptr} {
  if (ptr != nullptr) {
    count_ = new hstl::counter_t<Y*>(ptr);
  }
};
```

#### `make_shared<T>(Args)`

通过make_shared构造时，需要使用完美转发：

```c++
template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  shared_ptr<T> p;
  auto count = new counter_emplace<T>(hstl::forward<Args>(args)...);
  p.ptr_ = count->get_value_ptr();
  p.count_ = count;
  return p;
}
```

这里需要手动设置指针，不能`shared_ptr<T>(count->get_value_ptr())`这样设置，会重复增加计数。

### 赋值运算符

采用CopyAndSwap Idiom保证异常安全：

```c++
shared_ptr& operator=(const shared_ptr& other) noexcept {
  shared_ptr(other).swap(*this);
  return *this;
}
```

### 析构函数

这里的acquire内存屏障防止对象的析构函数重排序到前面执行。

```c++
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
```

可以和decrease_shared中的release合并为acq_rel次序，但是那样会增加开销，现在的实现中，acquire仅作用于if里面的代码。

当`shared_count_ == 0`，不会有其他线程再访问ptr\_，使得shared\_count_再增长。因此可以非原子地操作decr_shared和release_object。

### shared_ptr\<void\>

常用于C part的类型擦除。

#### operator*

```c++
T& operator*() { return *ptr_; }
```

当T为void时会编译出错，采用type_traits解决：

```c++
template <typename T>
struct shared_ptr_traits { using reference_type = T&; };
template <>
struct shared_ptr_traits<void> { using reference_type = void; };
template <>
struct shared_ptr_traits<void const> { using reference_type = void; };
template <>
struct shared_ptr_traits<void volatile> { using reference_type = void; };
template <>
struct shared_ptr_traits<void const volatile> { using reference_type = void; };
```

在类中使用type_traits：

```c++
using reference_type = typename shared_ptr_traits<T>::reference_type;
```

重新定义函数，此时返回void：

```c++
reference_type operator*() { return *ptr_; }
```

这个函数在T为void时在语义上就是不存在的，在运行时当然不能调用，这里只是保证编译不出错。

### shared_ptr\<T[]\>

#### operator[]

采用SFINE保证只有在参数为数组时函数才会生成。

```c++
// 做法1
template <typename U>
using enable_if_array = hstl::enable_if_t<hstl::is_array<U>::value>;

template <typename U = T, typename = enable_if_array<U>>
reference_type operator[](std::ptrdiff_t idx) const {
  return ptr_[idx];
}

// 做法2
reference_type operator[](std::ptrdiff_t idx) const {
  static_assert(hstl::is_array<T>::value, "T must be array type");
  return ptr_[idx];
}
```

注意，`typename U = T`必须存在，否则无法通过编译

TODO(hao): 为什么？

## weak_ptr

### 析构函数

```c++
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
```

采用初始化weak\_count\_为1的方式后，这里不需要再判断shared\_count\_，因为如果weak\_count_为0，一定不会有shared_ptr还引用对象。

### lock

当想要通过weak_ptr访问对象，必须先获得管理权，即先拷贝一个shared_ptr才行，并且只有在`shared_count_ > 0` 时才能这样做：

```c++
shared_ptr<T> lock() const noexcept {
  shared_ptr<T> p;

  auto shared = use_count();
  while (shared > 0) {
    if (count_->shared_count_.compare_exchange_weak(shared, shared + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
      p.ptr_ = ptr_;
      p.count_ = count_;
      break;
    }
    shared = use_count();
  }
  return p;
}
```

检查计数大于0后不能直接`incr_shared`，检查和递增不是一个原子操作，检查后有可能其他线程正好又减少了计数，这里采用`compare_exchange_weak`的方式不断尝试，直到成功。

检查计数大于0后不能直接`reuturn shared_ptr<T>(*this)`，同样是原子操作的原因。

#### compare_exchange_*比较，使用场景

在某些弱一致性的芯片上，strong版本开销很大，此时采用weak版本比较合适，weak版本的性能通常比较好。



## unique_ptr
