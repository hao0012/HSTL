#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <queue>
#include <iostream>

namespace hstl {
 
class ThreadPool {
public:
  using Task = std::function<void()>;

  ThreadPool(size_t thread_num): threads_(thread_num), closed_(false) {
    for (auto &t : threads_) {
      t = std::thread([this]() {
        while (!closed_.load(std::memory_order_acquire)) { // sync point
          Task f = nullptr;
          {
            std::unique_lock<std::mutex> guard(lock_);
            cv_.wait(guard, [this]() { return closed_.load(std::memory_order_relaxed) || !q_.empty(); });
            if (q_.empty()) {
              continue;
            }
            f = std::move(q_.front());
            q_.pop();
          }
          if (f != nullptr) {
            try {
              f();
            } catch (const std::exception& e) {
              std::cerr << "Task execution error: " << e.what() << std::endl;
            } catch (...) {
              std::cerr << "Unknown error occurred during task execution" << std::endl;
            }
          }
        }
    });
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> guard(lock_);
      closed_.store(true, std::memory_order_release); // sync point
    }
    
    cv_.notify_all();
    for (auto & t: threads_) {
      if (t.joinable()) {
        t.join();
      }
    }
  }

  template<typename F, typename... Args, typename R = std::invoke_result_t<F, Args...>>
  auto submit(F&& f, Args&&... args) -> std::future<R>;
  
  bool is_closed() const { return closed_.load(std::memory_order_acquire); }
  void close() {
    std::unique_lock<std::mutex> guard(lock_);
    closed_.store(true, std::memory_order_release); // sync point
  }

private:
  std::atomic<bool> closed_;
  
  std::mutex lock_;
  std::condition_variable cv_;

  std::queue<Task> q_;
  std::vector<std::thread> threads_;
};

template<typename F, typename... Args, typename R>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<R> {
  auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  auto p = std::make_shared<std::packaged_task<R()>>(func);
  auto future = p->get_future();

  if (closed_.load(std::memory_order_acquire)) {
    throw std::runtime_error("Cannot submit task to closed ThreadPool");
  }

  {
    std::scoped_lock guard(lock_);
    q_.push([=]() { (*p)(); });
  }

  cv_.notify_one();
  return future;
}

}

#endif // THREAD_POOL_H_