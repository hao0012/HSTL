#include <gtest/gtest.h>

#include <iostream>
// #include <thread>
// #include <vector>
#include "shared_ptr.hpp"
#include "enable_shared.hpp"

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

struct Base {
  Base() : b(new int[10]) {}
  virtual void print() const { std::cout << "Base" << std::endl; }
  virtual ~Base() {
    delete[] b;
    b = nullptr;
  }

  int* b;
};
struct Derived : public Base {
  Derived() : Base(), d(new int[10]) {}
  void print() const override { std::cout << "Derived" << std::endl; }
  ~Derived() {
    delete[] d;
    d = nullptr;
  }

  int* d;
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

TEST(SharedPtrTest, DeleterBasicTest) {
  bool deleter_set = false;
  auto deleter = [&](TestObject* p) {
    deleter_set = true;
    delete p;
  };

  {
    hstl::shared_ptr<TestObject> ptr(new TestObject(100), deleter);
    ASSERT_EQ(ptr.use_count(), 1);
    ASSERT_EQ(deleter_set, false);
  };
  ASSERT_EQ(deleter_set, true);
  deleter_set = false;
  {
    hstl::shared_ptr<TestObject> ptr(new TestObject(100), deleter);
    ASSERT_EQ(ptr.use_count(), 1);
    ASSERT_EQ(ptr->value, 100);
  }
  ASSERT_EQ(deleter_set, true);
}

TEST(SharedPtrTest, DeleterInheritanceTest) {
  auto bd = [](Base* p) {
    std::cout << "Delete Base" << std::endl;
    delete p;
  };
  auto dd = [](Derived* p) {
    std::cout << "Delete Derived" << std::endl;
    delete p;
  };
  hstl::shared_ptr<Derived> dp(new Derived(), dd);
  hstl::shared_ptr<Base> bp(dp);
}

struct EnableSharedTest : hstl::enable_shared_from_this<EnableSharedTest> {
  hstl::shared_ptr<EnableSharedTest> get_shared() { 
    return shared_from_this(); 
  }
};

TEST(SharedPtrTest, EnableSharedFromThisTest) {
  auto ptr = hstl::shared_ptr<EnableSharedTest>(new EnableSharedTest());
  ASSERT_EQ(ptr.use_count(), 1);
  auto ptr2 = ptr->get_shared();
  ASSERT_EQ(ptr.use_count(), 2);
  ASSERT_EQ(ptr2.use_count(), 2);

  auto ms_ptr = hstl::make_shared<EnableSharedTest>();
  ASSERT_EQ(ms_ptr.use_count(), 1);
  auto ms_ptr2 = ms_ptr->get_shared();
  ASSERT_EQ(ms_ptr.use_count(), 2);
  ASSERT_EQ(ms_ptr2.use_count(), 2);
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