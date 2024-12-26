#include <gtest/gtest.h>

#include <iostream>
// #include <thread>
// #include <vector>
#include "shared_ptr.hpp"

class TestObject {
 public:
  int value;
  explicit TestObject(int v) : value(v) {}
};

TEST(SharedPtrTest, VoidTest) {
  hstl::shared_ptr<void> ptr;
  ASSERT_EQ(ptr.use_count(), 0);
}

TEST(SharedPtrTest, BasicTest) {
  auto ptr1 = hstl::make_shared<TestObject>(10);
  ASSERT_EQ(ptr1.use_count(), 1);
  ASSERT_EQ(ptr1->value, 10);
  {
    auto ptr2 = ptr1;
    ASSERT_EQ(ptr1.use_count(), 2);
    ASSERT_EQ(ptr2.use_count(), 2);
    ASSERT_EQ(ptr2->value, 10);
  }
  ASSERT_EQ(ptr1.use_count(), 1);
}

struct SetDestroy {
  bool* destroyed;
  explicit SetDestroy(bool* d) : destroyed(d) {}
  ~SetDestroy() { *destroyed = true; }
};

TEST(SharedPtrTest, DestroyTest) {
  bool destroyed = false;
  {
    auto ptr = hstl::make_shared<SetDestroy>(&destroyed);
    ASSERT_EQ(destroyed, false);
  }

  ASSERT_EQ(destroyed, true);
}

TEST(SharedPtrTest, MoveTest) {
  auto ptr1 = hstl::make_shared<TestObject>(50);
  ASSERT_EQ(ptr1.use_count(), 1);

  auto ptr2 = hstl::move(ptr1);

  // ASSERT_EQ(ptr1.get(), nullptr);
  ASSERT_EQ(ptr1.use_count(), 0);
  ASSERT_EQ(ptr2->value, 50);
  ASSERT_EQ(ptr2.use_count(), 1);
}

class Base {
 public:
  virtual ~Base() = default;
  virtual void print() const { std::cout << "Base" << std::endl; }
};
class Derived : public Base {
 public:
  void print() const override { std::cout << "Derived" << std::endl; }
};

TEST(SharedPtrTest, InheritanceTest) {
  hstl::shared_ptr<Base> tmp = hstl::make_shared<Derived>();

  ASSERT_EQ(tmp.use_count(), 1);

  hstl::shared_ptr<Base> bp = tmp;
  ASSERT_EQ(tmp.use_count(), 2);
  ASSERT_EQ(bp.use_count(), 2);
  tmp.reset();
  ASSERT_EQ(bp.use_count(), 1);
  ASSERT_EQ(tmp.use_count(), 0);  // ptr2 已经被释放，里面的计数器设为nullptr

  // 确保可以调用派生类的方法
  {
    std::ostringstream oss;
    auto cout_buf = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());  // 重定向标准输出
    bp->print();
    std::cout.rdbuf(cout_buf);  // 恢复标准输出
    ASSERT_EQ(oss.str(), "Derived\n");
  }

  bp = hstl::make_shared<Base>();
  ASSERT_EQ(bp.use_count(), 1);
  {
    std::ostringstream oss;
    auto cout_buf = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());
    bp->print();
    std::cout.rdbuf(cout_buf);
    ASSERT_EQ(oss.str(), "Base\n");
  }
}

TEST(WeakPtrTest, BasicTest) {
  auto ptr = hstl::make_shared<TestObject>(10);
  ASSERT_EQ(ptr.use_count(), 1);  // 共享指针的引用计数应该是1

  hstl::weak_ptr<TestObject> wp(ptr);
  ASSERT_EQ(ptr.use_count(), 1);

  {
    ASSERT_EQ(ptr.use_count(), 1);
    hstl::shared_ptr<TestObject> ptr2 = wp.lock();
    ASSERT_EQ(ptr.use_count(), 2);
  }
  ASSERT_EQ(ptr.use_count(), 1);

  {
    hstl::weak_ptr<TestObject> wp2 = ptr;
    ASSERT_EQ(ptr.use_count(), 1);
  }
  ASSERT_EQ(ptr.use_count(), 1);

  ptr.reset();
  ASSERT_EQ(ptr.use_count(), 0);
  {
    hstl::shared_ptr<TestObject> ptr2 = wp.lock();
    ASSERT_EQ(ptr2.get(), nullptr);
  }
}