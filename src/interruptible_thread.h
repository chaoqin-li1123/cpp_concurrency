#ifndef INTERRUPTIBLE_THREAD
#define INTERRUPTIBLE_THREAD

#include <thread>
#include <atomic>
#include <future>

class InterruptFlag {
public:
  void set() {
    flag.store(true);
  }
  bool isSet() const {
    return flag;
  }
private:
  std::atomic<bool> flag{false};
};

thread_local InterruptFlag this_thread_interrupt_flag;

struct InterruptibleThread {
public:
  template <typename Callable>
  InterruptibleThread(Callable func) {
    std::promise<InterruptFlag*> flag;
    internal_thread_ = std::thread([func, &flag] {
      flag.set_value(&this_thread_interrupt_flag);
      func();
    });
    flag_ = flag.get_future().get();
  }

  void join() {

  }

  void detach() {

  }

  bool joinable() {
    return internal_thread_.joinable();
  }

  void interrupt() {
    if (flag_) {
      flag_->set();
    }
  }
private:
  std::thread internal_thread_;
  InterruptFlag* flag_;
};

#endif