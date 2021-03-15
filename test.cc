#include "thread_pool.h"
#include "thread_safe_container.h"
#include "interruptible_thread.h"
#include <chrono>
#include <future>
#include <iostream>

void testThreadPool() {
  Container::ThreadSafeHashMap<int, int> dict;
  std::atomic<int> cnt;
  cnt.store(0);
  Thread::ThreadPoolImpl pool;
  pool.start();
  std::vector<std::future<int>> rets;
  for (int i = 0; i < 10000; i++) {

    rets.push_back(pool.submit(
        [&dict, i] () {dict.put(i, i); return i;}
    ));


  }
  // pool.runOnAllThreads([] () {int x = 3;});
  pool.shutdown();
  for (int i = 0; i < 10000; i++) dict.erase(i);
  // dict.clear();
  std::cout << dict.size() << std::endl;
}

int main() { testThreadPool(); }