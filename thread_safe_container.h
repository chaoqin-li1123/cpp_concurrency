#ifndef THREADSAFESTACK
#define THREADSAFESTACK

#include <cassert>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stack>
#include <utility>
#include <vector>
#include <atomic>

namespace Container {

template <typename T> class ThreadSafeStack {
public:
  ThreadSafeStack() = default;

  void push(T &&t) {
    std::unique_lock<std::mutex> lck(mtx_);
    data_.push(std::move(t));
  }

  void push(T const &t) {
    std::unique_lock<std::mutex> lck(mtx_);
    data_.push(t);
    cv_.notify_one();
  }

  std::unique_ptr<T> blocking_pop() {
    std::unique_lock<std::mutex> lck;
    cv_.wait(lck, [this] { return !empty(); });
    std::unique_ptr<T> t{new T(std::move(data_.top()))};
    data_.pop();
    return t;
  }

  void blocking_pop(T &t) {
    std::unique_lock<std::mutex> lck;
    cv_.wait(lck, [this] { return !empty(); });
    t = std::move(data_.top());
    data_.pop();
    return t;
  }

  std::unique_ptr<T> try_pop() {
    std::unique_lock<std::mutex> lck(mtx_);
    if (data_.empty())
      return nullptr;
    std::unique_ptr<T> t{new T(std::move(data_.top()))};
    data_.pop();
    return t;
  }

  bool try_pop(T &t) {
    std::unique_lock<std::mutex> lck(mtx_);
    if (data_.empty())
      return false;
    t = move(data_.top());
    data_.pop();
    return true;
  }

  bool empty() const {
    std::unique_lock<std::mutex> lck(mtx_);
    return data_.empty();
  }

  void size() const {
    std::unique_lock<std::mutex> lck(mtx_);
    return data_.size();
  }

private:
  mutable std::mutex mtx_;
  std::condition_variable cv_;
  std::stack<T> data_;
};

template <typename T> class ThreadSafeQueue {
public:
  ThreadSafeQueue() : head_{new Node()}, tail_{head_.get()} {}
  void push(T const &t) {
    std::unique_lock<std::mutex> tail_lock(tail_mutex_);
    assert(tail_->data_ == nullptr);
    std::unique_ptr<Node> node{new Node()};
    Node *new_tail = node.get();
    tail_->data_ = std::make_unique<T>(t);
    tail_->next_ = move(node);
    tail_ = new_tail;
  }

  void push(T &&t) {
    std::unique_lock<std::mutex> tail_lock(tail_mutex_);
    assert(tail_->data_ == nullptr);
    std::unique_ptr<Node> node{new Node()};
    Node *new_tail = node.get();
    tail_->data_ = std::make_unique<T>(move(t));
    tail_->next_ = move(node);
    tail_ = new_tail;
  }

  std::unique_lock<T> try_pop() {
    std::unique_ptr<Node> old_head = pop_head();
    if (old_head == nullptr)
      return nullptr;
    std::unique_ptr<T> data = move(old_head->data_);
    return data;
  }

  bool try_pop(T &t) {
    std::unique_ptr<Node> old_head = pop_head();
    if (old_head == nullptr)
      return false;
    t = *(old_head->data_.release());
    return true;
  }

  bool empty() {
    std::unique_lock<std::mutex> head_lock(head_mutex_);
    return head_.get() == getTail();
  }

private:
  class Node {
  public:
    std::unique_ptr<T> data_;
    std::unique_ptr<Node> next_{nullptr};
  };
  Node *getTail() {
    std::unique_lock<std::mutex> tail_lock(tail_mutex_);
    return tail_;
  }
  std::unique_ptr<Node> pop_head() {
    std::unique_lock<std::mutex> head_lock(head_mutex_);
    if (head_.get() == getTail())
      return nullptr;
    std::unique_ptr<Node> old_head = move(head_);
    head_ = std::move(old_head->next_);
    return old_head;
  }
  std::unique_ptr<Node> head_;
  std::mutex head_mutex_;
  std::mutex tail_mutex_;
  Node *tail_;
};

template <typename Key, typename Value> class ThreadSafeHashMap {
public:
  ThreadSafeHashMap() : bucket_cnt_{19} { buckets_ = new Bucket[bucket_cnt_]; }

  ThreadSafeHashMap(size_t bucket_cnt) : bucket_cnt_{bucket_cnt} {
    buckets_ = new Bucket[bucket_cnt_];
  }
  ~ThreadSafeHashMap() {
    delete [] buckets_;
  }
  int getBucketId(Key const &key) { return std::hash<Key>{}(key); }

  void put(Key const &key, Value const &value) {
    std::shared_lock<std::shared_timed_mutex> shared_lock(mtx_);
    int idx = getBucketId(key) % bucket_cnt_;
    if (buckets_[idx].put(key, value))
      size_++;
    if (size_ > 50 && size_  > 10 * bucket_cnt_) {
      shared_lock.unlock();
      resize(size_ / 3);
    }
  }

  void erase(Key const &key) {
    std::shared_lock<std::shared_timed_mutex> shared_lock(mtx_);
    int idx = getBucketId(key) % bucket_cnt_;
    if (buckets_[idx].erase(key))
      size_--;
    if (size_ > 50 && size_ < 5 * bucket_cnt_) {
      shared_lock.unlock();
      resize(size_ / 3);
    }
  }

  Value get(Key const &key) {
    std::shared_lock<std::shared_timed_mutex> shared_lock(mtx_);
    int idx = getBucketId(key) % bucket_cnt_;
    return buckets_[idx].get(key);
  }

  void resize(int bucket_cnt) {
    std::unique_lock<std::shared_timed_mutex> exclusive_lock(mtx_);
    Bucket* buckets = new Bucket[bucket_cnt];
    for (int i = 0; i < bucket_cnt_; i++) {
      for (auto& kv: buckets_[i].kvs_) {
        int idx = getBucketId(kv.first) % bucket_cnt;
        buckets[idx].put(kv.first, kv.second);
      }
    }
    std::swap(buckets_, buckets);
    delete [] buckets;
    bucket_cnt_ = bucket_cnt;
  }

  void clear() {
    std::unique_lock<std::shared_timed_mutex> exclusive_lock(mtx_);
    for (int i = 0; i < bucket_cnt_; i++) {
      buckets_[i].kvs_.clear();
    }    
    size_.store(0);
  }

  size_t size() {
    return size_;
  }

private:
  class Bucket {
    public:
    // return true if a new pair is added
    bool put(Key const &key, Value const &value) {
      std::unique_lock<std::shared_timed_mutex> write_lock(mtx_);
      for (auto &kv : kvs_) {
        if (kv.first == key) {
          kv.second = value;
          return false;
        }
      }
      kvs_.emplace_back(key, value);
      return true;
    }
    // return true if a pair is erased.
    bool erase(Key const &key) {
      std::unique_lock<std::shared_timed_mutex> write_lock(mtx_);
      for (auto it = kvs_.begin(); it != kvs_.end(); it++) {
        if (it->first == key) {
          kvs_.erase(it);
          return true;
        }
      }
      return false;
    }
    Value get(Key const &key) {
      std::shared_lock<std::shared_timed_mutex> read_lock(mtx_);
      for (auto &kv : kvs_) {
        if (kv.first == key)
          return kv.second;
      }
      return Value();
    }
    std::shared_timed_mutex mtx_;
    std::list<std::pair<Key, Value>> kvs_;
  };
  size_t bucket_cnt_;
  std::atomic<size_t> size_{0};
  Bucket* buckets_;
  std::shared_timed_mutex mtx_;
};

} // namespace Container

#endif
