#include "thread_pool.h"
#include "thread_safe_container.h"
#include "interruptible_thread.h"
#include "parallel_algo.h"
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
  /*
  for (int i = 0; i < 10000; i++) {
    rets.push_back(pool.submit(
        [&dict, i] () {dict.put(i, i); return i;}
    ));
  }
  */
  std::vector<int> arr{31, 23, 5, 7, 44};
  std::list<int> arr1{31, 23, 5, 5, 7, 44, 1};
  Parallel::SortVector<int>(arr, 0, arr.size() - 1);
  arr1 = Parallel::SortList<int>(arr1);
  for (int x : arr1) std::cout << x << ",";
  std::cout << std::endl;
  // pool.runOnAllThreads([] () {int x = 3;});
  pool.shutdown();
  for (int i = 0; i < 10000; i++) dict.erase(i);
  // dict.clear();
  std::cout << dict.size() << std::endl;
}

int main() { testThreadPool(); }