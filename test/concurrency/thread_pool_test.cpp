#include <gtest/gtest.h>
#include "concurrency/thread_pool.h"

TEST(THREAD_POOL_TEST, TEST0) {
  hstl::ThreadPool pool(10);
  pool.start();

  auto add = [](int a, int b) { return a + b; };

  auto future = pool.submit(add, 1, 2);

  ASSERT_EQ(3, future.get());
  
  pool.close();
}

TEST(ThreadPoolTest, TEST1) {
  hstl::ThreadPool pool(4);
  pool.start();
  
  std::atomic<int> counter(0);
  const int task_num = 100;
  
  std::vector<std::future<void>> futures;
  for (int i = 0; i < task_num; ++i) {
    futures.push_back(pool.submit([&counter]() {
      counter.fetch_add(1, std::memory_order_relaxed);
    }));
  }
  
  for (auto& fut : futures) {
    fut.get();
  }
  
  EXPECT_EQ(counter.load(), task_num);
}

TEST(ThreadPoolTest, TEST2) {
  hstl::ThreadPool pool(2);
  pool.start();
  pool.close();
  
  EXPECT_THROW(
    pool.submit([]() { std::cout << "This task should not run" << std::endl; }),
    std::runtime_error
  );
}

TEST(ThreadPoolTest, TEST3) {
  std::atomic<bool> task_started(false);
  
  {
    hstl::ThreadPool pool(1);
    pool.start();
    
    auto fut = pool.submit([&task_started]() {
      task_started.store(true, std::memory_order_relaxed);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    
    while (!task_started.load()) {
      std::this_thread::yield();
    }
  }
  
  EXPECT_TRUE(true);
}

TEST(ThreadPoolTest, TEST4) {
  const int thread_num = 8;
  const int task_num = 1000;
  hstl::ThreadPool pool(thread_num);
  pool.start();
  
  std::atomic<long long> sum(0);
  std::vector<std::future<void>> futures;
  
  for (int i = 0; i < task_num; ++i) {
    futures.push_back(pool.submit([&sum, i]() {
      sum.fetch_add(i, std::memory_order_relaxed);
    }));
  }
  
  for (auto& fut : futures) {
    fut.get();
  }
  
  EXPECT_EQ(sum.load(), (task_num - 1) * task_num / 2);
}

TEST(ThreadPoolTest, TEST5) {
  hstl::ThreadPool pool(1);
  pool.start();
  
  auto fut = pool.submit([]() {
    throw std::runtime_error("Test exception");
  });
  
  EXPECT_THROW(fut.get(), std::runtime_error);
  
  auto fut2 = pool.submit([]() { return 100; });
  EXPECT_EQ(fut2.get(), 100);
}