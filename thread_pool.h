#include "thread_safe_container.h"
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <exception>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace Thread {
using Task = std::function<void()>;
class ThreadPool {
public:
  virtual ~ThreadPool() = default;
  virtual bool addTask(Task const &task) = 0;
  virtual bool addTask(Task &&task) = 0;
  virtual void shutdown() = 0;
  virtual void start() = 0;
};
namespace v1 {

class ThreadPoolImpl : public ThreadPool {
public:
  ThreadPoolImpl(size_t threads) : threads_{threads} {}

  ThreadPoolImpl() : threads_{std::thread::hardware_concurrency()} {}

  ~ThreadPoolImpl() override { shutdown(); }
  void start() override {
    shutdown_.store(false);
    for (int i = 0; i < threads_; i++) {
      workers_.emplace_back(&ThreadPoolImpl::worker, this);
    }
  }

  void shutdown() override {
    if (shutdown_.load())
      return;
    shutdown_.store(true);
    cv_.notify_all();
    for (auto &worker : workers_) {
      worker.join();
    }
  }

  bool addTask(Task &&task) override {
    std::unique_lock<std::mutex> lck(mtx_);
    if (shutdown_.load())
      return false;
    tasks_.push_back(move(task));
    lck.unlock();
    cv_.notify_one();
    return true;
  }

  bool addTask(Task const &task) override {
    std::unique_lock<std::mutex> lck(mtx_);
    if (shutdown_.load())
      return false;
    tasks_.push_back(task);
    lck.unlock();
    cv_.notify_one();
    return true;
  }

private:
  void worker() {
    while (true) {
      std::unique_lock<std::mutex> lck(mtx_);
      cv_.wait(lck, [this] { return !tasks_.empty() || shutdown_.load(); });
      if (shutdown_ && tasks_.empty()) {
        lck.unlock();
        cv_.notify_all();
        break;
      }
      Task task = move(tasks_.front());
      tasks_.pop_front();
      lck.unlock();
      cv_.notify_one();
      task();
    }
  }

  std::condition_variable cv_;
  std::mutex mtx_;
  std::list<Task> tasks_;
  size_t threads_;
  std::atomic<bool> shutdown_;
  std::list<std::thread> workers_;
};
} // namespace v1

namespace v2 {
class ThreadPoolImpl : public ThreadPool {
public:
  ThreadPoolImpl() : threads_{std::thread::hardware_concurrency()} {
    shutdown_.store(true);
  }

  ThreadPoolImpl(size_t threads) : threads_{threads} { shutdown_.store(true); }

  ~ThreadPoolImpl() override { shutdown(); }

  void start() override {
    shutdown_.store(false);
    for (int i = 0; i < threads_; i++) {
      workers_.emplace_back(&ThreadPoolImpl::worker, this);
    }
  }

  void shutdown() override {
    if (shutdown_.load())
      return;
    shutdown_.store(true);
    for (auto &worker : workers_)
      worker.join();
  }

  bool addTask(Task const &task) override {
    if (shutdown_.load())
      return false;
    tasks_.push(task);
    return true;
  }

  bool addTask(Task &&task) override {
    if (shutdown_.load())
      return false;
    tasks_.push(move(task));
    return true;
  }

private:
  void worker() {
    while (true) {
      if (shutdown_.load() && tasks_.empty())
        break;
      Task task;
      if (tasks_.try_pop(task)) {
        task();
      } else {
        std::this_thread::yield();
      }
    }
  }

  Container::ThreadSafeStack<Task> tasks_;
  std::atomic<bool> shutdown_;
  size_t threads_;
  std::list<std::thread> workers_;
};
} // namespace v2
using namespace v2;

} // namespace Thread
