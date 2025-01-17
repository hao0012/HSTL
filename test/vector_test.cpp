#include <gtest/gtest.h>
#include <vector>

#include "vector.hpp"

TEST(VectorTest, BasicTest) {
  hstl::vector<int> vec;
  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 0);

  for (int i = 0; i < 10; ++i) {
    vec.push_back(i);
  }

  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(vec[i], i);
  }
  ASSERT_EQ(vec.size(), 10);
  ASSERT_EQ(vec.capacity(), 16);

  hstl::vector<int> vec2(10, 1);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(vec2[i], 1);
  }
}

struct CopyMoveFoo {
  static int move_ctor_count;
  static int copy_ctor_count;
  
  int value_;

  explicit CopyMoveFoo(int v): value_(v) {}
  CopyMoveFoo(const CopyMoveFoo& f) : value_(f.value_) { ++copy_ctor_count; }
  CopyMoveFoo& operator=(const CopyMoveFoo& f) { value_ = f.value_; return *this; }
  CopyMoveFoo(CopyMoveFoo&& f) : value_(f.value_) { ++move_ctor_count; }
  CopyMoveFoo& operator=(CopyMoveFoo&& f) { value_ = f.value_; return *this; }
};

int CopyMoveFoo::copy_ctor_count = 0;
int CopyMoveFoo::move_ctor_count = 0;

TEST(VectorTest, MoveFirstTest) {
  CopyMoveFoo::copy_ctor_count = 0;
  CopyMoveFoo::move_ctor_count = 0;

  hstl::vector<CopyMoveFoo> vec;
  // 添加新元素以及扩容时的拷贝优先使用移动构造函数

  // 扩容后在新的内存上移动构造对象
  vec.push_back(CopyMoveFoo(1));
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 1);
  // 扩容后先移动构造vec[0]，然后移动构造vec[1]
  vec.push_back(CopyMoveFoo(2));
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 1 + 2);
  // 扩容后先移动构造vec[0 1]，然后移动构造vec[2]
  vec.push_back(CopyMoveFoo(3));
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 1 + 2 + 3);
  
  ASSERT_EQ(CopyMoveFoo::copy_ctor_count, 0);
  
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(vec[i].value_, i + 1);
  }
  for (int i = 0; i < 3; ++i) {
    vec.pop_back();
  }
  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 4);
}

TEST(VectorTest, EmplaceBackTest) {
  CopyMoveFoo::move_ctor_count = 0;
  CopyMoveFoo::copy_ctor_count = 0;

  hstl::vector<CopyMoveFoo> vec;
  vec.emplace_back(1);
  vec.emplace_back(2);
  vec.emplace_back(3);
  // 与push_back相比减少了一次对象的构造
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 3);
  ASSERT_EQ(CopyMoveFoo::copy_ctor_count, 0);
}

TEST(VectorTest, InsertTest) {
  CopyMoveFoo::move_ctor_count = 0;
  CopyMoveFoo::copy_ctor_count = 0;

  hstl::vector<CopyMoveFoo> vec;
  vec.insert(vec.begin(), CopyMoveFoo(1));
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 1);

  vec.insert(vec.begin(), CopyMoveFoo(2));
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 1 + 2);

  vec.insert(vec.begin(), CopyMoveFoo(3));
  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 1 + 2 + 3);

  vec.insert(vec.begin() + 1, CopyMoveFoo(4));
  ASSERT_EQ(vec[0].value_, 3);
  ASSERT_EQ(vec[1].value_, 4);
  ASSERT_EQ(vec[2].value_, 2);
  ASSERT_EQ(vec[3].value_, 1);

  ASSERT_EQ(vec.size(), 4);
  ASSERT_EQ(vec.capacity(), 4);
}

TEST(VectorTest, EmplaceTest) {
  CopyMoveFoo::move_ctor_count = 0;
  CopyMoveFoo::copy_ctor_count = 0;

  hstl::vector<CopyMoveFoo> vec;
  vec.emplace(vec.begin(), 1);
  vec.emplace(vec.begin(), 2);
  vec.emplace(vec.begin(), 3);

  ASSERT_EQ(CopyMoveFoo::move_ctor_count, 3);
  ASSERT_EQ(CopyMoveFoo::copy_ctor_count, 0);

  vec.emplace(vec.begin() + 1, 4);
  ASSERT_EQ(vec[0].value_, 3);
  ASSERT_EQ(vec[1].value_, 4);
  ASSERT_EQ(vec[2].value_, 2);
  ASSERT_EQ(vec[3].value_, 1);

  ASSERT_EQ(vec.size(), 4);
  ASSERT_EQ(vec.capacity(), 4);
}

TEST(VectorTest, MultiInsertTest) {
  CopyMoveFoo::move_ctor_count = 0;
  CopyMoveFoo::copy_ctor_count = 0;

  hstl::vector<CopyMoveFoo> vec(3, CopyMoveFoo(1));
  ASSERT_EQ(vec.size(), 3);
  vec.insert(vec.begin() + 1, 3, CopyMoveFoo(2));
  ASSERT_EQ(vec.size(), 6);
  ASSERT_EQ(vec.capacity(), 6);
  ASSERT_EQ(vec[0].value_, 1);
  for (int i = 1; i < 4; ++i) {
    ASSERT_EQ(vec[i].value_, 2);
  }
  ASSERT_EQ(vec[4].value_, 1);
  ASSERT_EQ(vec[5].value_, 1);
  // TODO(hao) 接着写测试
}

TEST(VectorTest, ReserveTest) {
  CopyMoveFoo::move_ctor_count = 0;
  CopyMoveFoo::copy_ctor_count = 0;

  hstl::vector<CopyMoveFoo> vec;
  vec.reserve(10);
  ASSERT_EQ(vec.capacity(), 10);
  ASSERT_EQ(vec.size(), 0);

  vec.emplace_back(1);
  vec.emplace_back(2);
  vec.emplace_back(3);
  vec.reserve(10);
  ASSERT_EQ(vec.capacity(), 10);
  ASSERT_EQ(vec.size(), 3);

  vec.reserve(2);
  ASSERT_EQ(vec.capacity(), 10);
  ASSERT_EQ(vec.size(), 3);
}