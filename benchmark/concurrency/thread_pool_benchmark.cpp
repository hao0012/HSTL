#include "concurrency/thread_pool.h"

#include <chrono>
#include <future>
#include <vector>
#include <iostream>

static double lock_contention_benchmark(int thread_count, int task_count) {
  using namespace std::chrono_literals;
  auto add = [](int a, int b) { return a + b; };

  hstl::ThreadPool pool(thread_count);

  std::vector<std::future<int>> res(task_count);
  
  auto start_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < task_count; i++) {
    res[i] = pool.submit(add, i, i);
  }
  
  for (int i = 0; i < task_count; i++) {
    res[i].get();
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
}

int main() {
  std::vector<int> thread_counts = {1, 2, 3, 4, 8, 16, 32, 64, 128};
  int task_count = 1000000;

  int repeat_times = 10;

  for (int i = 0; i < thread_counts.size(); i++) {
    double average = 0;
    for (int j = 0; j < repeat_times; j++) {
      average += lock_contention_benchmark(thread_counts[i], task_count);
    }
    average /= repeat_times;
    std::cout << "thread_count: " << thread_counts[i] << ", task_count: " << task_count << ", average cost time: " << average << std::endl;
  }
}