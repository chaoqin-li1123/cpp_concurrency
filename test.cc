#include "thread_pool.h"
#include "thread_safe_container.h"
#include <chrono>
#include <iostream>


int main() {
    Container::ThreadSafeHashMap<int, int> dict;
    std::atomic<int> cnt;
    cnt.store(0);
    Thread::ThreadPoolImpl pool;
    pool.start();
    for (int i = 0; i < 10000; i++) pool.addTask([&dict, x = i] {
    dict.put(x, x);
    });
    pool.shutdown();
    dict.clear();
    std::cout << dict.size() << std::endl;
}