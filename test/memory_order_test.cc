#include <atomic>
#include <cassert>
#include <iostream>
#include <list>
#include <thread>

std::atomic<bool> x;
std::atomic<bool> y;
std::atomic<int> cnt;

void init() {
  x = false;
  y = false;
  cnt = 0;
}

void set_x(std::memory_order order) { x.store(true, order); }

void set_y(std::memory_order order) { y.store(true, order); }

void set_x_then_y(std::memory_order order) {
  set_x(order);
  set_y(order);
}

void get_x_then_y(std::memory_order order) {
  while (!x.load(order))
    ;
  if (y.load(order))
    cnt++;
}

void get_y_then_x(std::memory_order order) {
  while (!y.load(order))
    ;
  if (x.load(order))
    cnt++;
}

void seq_cst_test() {
  init();
  std::thread t3{get_x_then_y, std::memory_order_seq_cst},
      t4{get_y_then_x, std::memory_order_seq_cst},
      t1{set_x, std::memory_order_seq_cst},
      t2{set_y, std::memory_order_seq_cst};
  t1.join(), t2.join(), t3.join(), t4.join();
  assert(cnt.load() > 0);
  // std::cout << cnt.load() << std::endl;
}

void mem_relaxed_test() {
  init();
  std::thread 
      t1{get_y_then_x, std::memory_order_relaxed},
      t2{set_x_then_y, std::memory_order_relaxed};
  t2.join(), t1.join();
  assert(cnt.load() > 0);
}

void mem_acq_rel_test() {
  init();
  std::thread 
      t1{get_y_then_x, std::memory_order_acquire},
      t2{set_x_then_y, std::memory_order_release};
  t2.join(), t1.join();
  assert(cnt.load() > 0);  
}

template <typename Callable> void test_n_times(int n, Callable callable) {
  for (int i = 0; i < n; i++) {
    callable();
  }
}

std::atomic<int> data[3];

std::atomic<bool> sync1{false}, sync2{false};

void seq_set() {
  data[0].store(0, std::memory_order_acquire);
  data[1].store(1, std::memory_order_acquire);
  data[2].store(2, std::memory_order_acquire);
  sync1.store(true, std::memory_order_acquire);
}

void syn_bool() {
  while (!sync1.load(std::memory_order_release));
  sync2.store(true, std::memory_order_acquire);
}

void seq_get() {
  while (!sync2.load(std::memory_order_release));
  assert(data[0].load(std::memory_order_relaxed) == 0);
  assert(data[1].load(std::memory_order_relaxed) == 1);
  assert(data[2].load(std::memory_order_relaxed) == 2);
}

void transitive_order() {
  std::thread t1(seq_set), t2(syn_bool), t3(seq_get), t4(seq_get);
  t1.join(), t2.join(), t3.join(), t4.join();
}

int main() { test_n_times(1000, transitive_order); }