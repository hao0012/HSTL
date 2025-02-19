#include "unique_ptr.hpp"

#include <gtest/gtest.h>

#include <iostream>

TEST(UniquePtrTest, DefaultConstructor) {
  hstl::unique_ptr<int> ptr;
  EXPECT_EQ(ptr.get(), nullptr);
  hstl::unique_ptr<int> ptr2(nullptr);
  EXPECT_EQ(ptr2.get(), nullptr);
}

struct MoveOnly {
  int x;
  MoveOnly(int x) : x(x) {}
};

struct FooDeleter {
  void operator()(MoveOnly* foo) {
    std::cout << "delete foo..." << std::endl;
    delete foo;
  }
};

TEST(UniquePtrTest, ConstructorWithRawPointer) {
  hstl::unique_ptr<int> ptr(new int(42));
  EXPECT_NE(ptr.get(), nullptr);
  EXPECT_EQ(*ptr, 42);

  hstl::unique_ptr<MoveOnly, FooDeleter> fooPtr(new MoveOnly(42));
  EXPECT_NE(fooPtr.get(), nullptr);
  // print "delete foo..."
}

TEST(UniquePtrTest, MoveConstructor) {
  hstl::unique_ptr<int> ptr1(new int(42));
  hstl::unique_ptr<int> ptr2(std::move(ptr1));
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_NE(ptr2.get(), nullptr);
  EXPECT_EQ(*ptr2, 42);

  hstl::unique_ptr<MoveOnly, FooDeleter> fooPtr1(new MoveOnly(42), FooDeleter());
  hstl::unique_ptr<MoveOnly, FooDeleter> fooPtr2(hstl::move(fooPtr1));
  EXPECT_EQ(fooPtr1.get(), nullptr);
  // 注意，下面的测试是错误的，fooPtr2中会构造一个新的pair，
  // move在这里的作用是减少参数传递时的拷贝，而不是说fooPtr2
  // 中直接复用了fooPtr1的pair
  // auto deleter_pointer = &fooPtr1.get_deleter();
  // EXPECT_EQ(&fooPtr2.get_deleter(), deleter_pointer);
}

TEST(UniquePtrTest, MoveAssignment) {
  hstl::unique_ptr<int> ptr1(new int(42));
  hstl::unique_ptr<int> ptr2;
  ptr2 = std::move(ptr1);
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_NE(ptr2.get(), nullptr);
  EXPECT_EQ(*ptr2, 42);
}

TEST(UniquePtrTest, Release) {
  hstl::unique_ptr<int> ptr(new int(42));
  int* rawPtr = ptr.release();
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_EQ(*rawPtr, 42);
  delete rawPtr;
}

TEST(UniquePtrTest, Reset) {
  hstl::unique_ptr<int> ptr(new int(42));
  ptr.reset(new int(84));
  EXPECT_EQ(*ptr, 84);
}

TEST(UniquePtrTest, EBO) {
  hstl::unique_ptr<int> ptr1(new int(42));
  EXPECT_EQ(sizeof(ptr1), sizeof(int*));

  // 静态类型的删除器，编译时即可确定类型大小
  hstl::unique_ptr<MoveOnly, FooDeleter> ptr2(new MoveOnly(42));
  EXPECT_EQ(sizeof(ptr2), sizeof(MoveOnly*));

  // 函数指针的大小不为0，无法应用EBO
  // 动态类型的删除器，指针可以指向任意和签名匹配的类型，并且在运行时改变
  hstl::unique_ptr<MoveOnly, void (*)(MoveOnly*)> ptr3(new MoveOnly(42),
                                             [](MoveOnly* f) { delete f; });
  EXPECT_EQ(sizeof(ptr3), sizeof(MoveOnly*) + sizeof(void (*)(MoveOnly*)));
  // hstl::unique_ptr<Foo, decltype( [](Foo* f) { delete f; } )> ptr4(new
  // Foo(42),
  //                                         [](Foo* f) { delete f; });
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
  hstl::unique_ptr<Base> tmp = hstl::make_unique<Derived>();

  // 确保可以调用派生类的方法
  {
    std::ostringstream oss;
    auto cout_buf = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());  // 重定向标准输出
    tmp->print();
    std::cout.rdbuf(cout_buf);  // 恢复标准输出
    ASSERT_EQ(oss.str(), "Derived\n");
  }
}