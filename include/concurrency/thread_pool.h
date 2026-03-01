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

  ThreadPool(size_t thread_num): closed_(false), queues_(thread_num), threads_(thread_num) {
    for (int i = 0; i < thread_num; i++) {
      threads_[i] = std::thread([this, i]() {
        while (!closed_.load(std::memory_order_acquire)) { // sync point
          Task f = get_one_task(i);
          if (f == nullptr) {
            auto &queue = queues_[i];
            std::unique_lock guard(queue.lock);
            queue.cv.wait(guard, [this, &queue]() { return !queue.q.empty() || closed_.load(std::memory_order_relaxed); });
            continue;
          }
          
          try {
            f();
          } catch (const std::exception& e) {
            std::cerr << "Task execution error: " << e.what() << std::endl;
          } catch (...) {
            std::cerr << "Unknown error occurred during task execution" << std::endl;
          }
        }
    });
    }
  }

  ~ThreadPool() {
    for (auto & q: queues_) {
      {
        std::unique_lock<std::mutex> guard(q.lock);
        closed_.store(true, std::memory_order_release); // sync point
      }
      q.cv.notify_all();
    }
    
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
    for (auto &queue: queues_) {
      std::unique_lock<std::mutex> guard(queue.lock);
      closed_.store(true, std::memory_order_release); // sync point
    }
  }

private:
  std::atomic<bool> closed_;

  Task get_one_task(int thread_id) {
    auto &t = threads_[thread_id];
    auto &queue = queues_[thread_id];
    Task f = nullptr;
    {
      std::unique_lock guard(queue.lock);
      if (!queue.q.empty()) {
        f = std::move(queue.q.front());
        queue.q.pop();
        return f;
      }
    }

    for (int i = 0; i < threads_.size(); i++) {
      if (i == thread_id) {
        continue;
      }
      
      {
        std::unique_lock guard(queue.lock);
        if (!queue.q.empty()) {
          f = std::move(queue.q.front());
          queue.q.pop();
          break;
        }
      }
    }

    return f;
  }
  
  struct cv_queue {
    std::mutex lock;
    std::condition_variable cv;

    std::queue<Task> q;
  };

  std::vector<cv_queue> queues_;
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

  static std::mutex i_lock;
  static int i = 0;
  
  i_lock.lock();
  auto &queue = queues_[i];
  i = (i + 1) % queues_.size();
  i_lock.unlock();
  {
    std::unique_lock guard(queue.lock);
    queue.q.push([=]() { (*p)(); });
  }

  queue.cv.notify_one();
  return future;
}

}

#endif // THREAD_POOL_H_