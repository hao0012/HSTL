#include <gtest/gtest.h>

#include "unique_ptr.hpp"

TEST(UniquePtrTest, DefaultConstructor) {
  hstl::unique_ptr<int> ptr;
  EXPECT_EQ(ptr.get(), nullptr);
}

TEST(UniquePtrTest, ConstructorWithRawPointer) {
  hstl::unique_ptr<int> ptr(new int(42));
  EXPECT_NE(ptr.get(), nullptr);
  EXPECT_EQ(*ptr, 42);
}

TEST(UniquePtrTest, MoveConstructor) {
  hstl::unique_ptr<int> ptr1(new int(42));
  hstl::unique_ptr<int> ptr2(std::move(ptr1));
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_NE(ptr2.get(), nullptr);
  EXPECT_EQ(*ptr2, 42);
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