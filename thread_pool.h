#include "thread_safe_container.h"
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <exception>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include <future>

namespace Thread {
using Task = std::function<void()>;
class ThreadPool {
public:
  virtual ~ThreadPool() = default;
  virtual bool addTask(Task const &task) = 0;
  virtual bool addTask(Task &&task) = 0;
  virtual void shutdown() = 0;
  virtual bool start() = 0;
};


class spinlockMutex {
public:
  spinlockMutex(): flag(ATOMIC_FLAG_INIT) {}
  void lock() {
    while (flag.test_and_set(std::memory_order_acquire));
  }
  void unlock() {
    flag.test_and_set(std::memory_order_release);
  }
private:
  std::atomic_flag flag;
};

namespace v1 {

class ThreadPoolImpl : public ThreadPool {
public:
  ThreadPoolImpl(size_t threads) : threads_{threads} {}

  ThreadPoolImpl() : threads_{std::thread::hardware_concurrency()} {}

  ~ThreadPoolImpl() override { shutdown(); }
  bool start() override {
    shutdown_.store(false);
    try {
      for (int i = 0; i < threads_; i++) {
        workers_.emplace_back(&ThreadPoolImpl::worker, this);
      }
    }
    catch (std::exception& e) {
      shutdown();
      return false;
    }
    return true;
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
    tasks_.push_back(std::move(task));
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
      Task task = std::move(tasks_.front());
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

  bool start() override {
    shutdown_.store(false);
    try {
      for (int i = 0; i < threads_; i++) {
        workers_.emplace_back(&ThreadPoolImpl::worker, this);
      }
    }
    catch (std::exception& e) {
      shutdown();
      return false;
    }
    return true;
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
    tasks_.push(std::move(task));
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

namespace v3 {
class Function{
  struct AbstractFunction {
    virtual ~AbstractFunction() {}
    virtual void operator()() = 0;
  };
public:
  template <typename F> struct ConcreteFunction : public AbstractFunction {
    F f_;
    ConcreteFunction(F &&f) : f_{std::move(f)} {}
    void operator()() override { f_(); }
  };
  Function() = default;
  template <typename F>
  Function(F&& f): f_{new ConcreteFunction<F>(std::move(f))} {}
  Function(Function&& other): f_{std::move(other.f_)} {}
  Function& operator=(Function&& other) {
    f_ = std::move(other.f_);
    return *this;
  }
  Function(Function const&) = delete;
  Function& operator=(Function const&) = delete;
  std::unique_ptr<AbstractFunction> f_;
  void operator()() { (*f_)(); }
};

class ThreadPoolImpl : public ThreadPool {
public:
  ThreadPoolImpl() : threads_{std::thread::hardware_concurrency()} {
    shutdown_.store(true);
  }

  ThreadPoolImpl(size_t threads) : threads_{threads} { shutdown_.store(true); }

  ~ThreadPoolImpl() override { shutdown(); }

  bool start() override {
    thread_local_tasks_.resize(threads_);
    shutdown_.store(false);
    try {
      for (int i = 0; i < threads_; i++) {
        workers_.emplace_back(&ThreadPoolImpl::worker, this);
      }
    }
    catch (std::exception& e) {
      shutdown();
      return false;
    }
    return true;
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
    tasks_.push(std::move(task));
    return true;
  }

  template<typename F>
  std::future<typename std::result_of<F()>::type> 
  submit(F const& f) {
    using ResultType = typename std::result_of<F()>::type;
    if (shutdown_.load()) return {};
    std::packaged_task<ResultType()> task(std::move(f));
    std::future<ResultType> result(task.get_future());
    if (thread_id_ >= 0) {
      thread_local_tasks_[thread_id_].push_back(std::move(task));
    }
    else {
      tasks_.push(std::move(task));
    }
    return result;
  }

  template<typename F>
  std::future<typename std::result_of<F()>::type> 
  submit(F && f) {
    using ResultType = typename std::result_of<F()>::type;
    std::packaged_task<ResultType()> task(std::move(f));
    std::future<ResultType> result(task.get_future());
    if (thread_id_ >= 0) {
      thread_local_tasks_[thread_id_].push_back(std::move(task));
    }
    else {
      tasks_.push(std::move(task));
    }
    return result;
  }

  void runPendingTask() {
      Function task;
      if (thread_id_ >= 0 && !thread_local_tasks_[thread_id_].empty()) {
        task = std::move(thread_local_tasks_[thread_id_].front());
        thread_local_tasks_[thread_id_].pop_front();
        task();
      }
      else if (tasks_.try_pop(task)) {
        task();
      } else {
        std::this_thread::yield();
      }
  }

  void runOnAllThreads(std::function<void()> f) {
    for (int i = 0; i < threads_; i++) {
      std::function<void()> temp = f;
      thread_local_tasks_[i].emplace_back(std::move(temp));
    }
  }

private:
  void worker() {
    thread_id_ = cur_id_++;
    while (true) {
      if (shutdown_.load() && tasks_.empty() && thread_local_tasks_[thread_id_].empty())
        break;
      runPendingTask();
    }
  }

  Container::ThreadSafeStack<Function> tasks_;
  std::vector<std::list<Function>> thread_local_tasks_;
  static thread_local int thread_id_;
  std::atomic<bool> shutdown_;
  int cur_id_ = 0;
  size_t threads_;
  std::list<std::thread> workers_;
};

thread_local int ThreadPoolImpl::thread_id_{-1};

} // namespace v3

using namespace v3;

} // namespace Thread
