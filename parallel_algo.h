#ifndef PARALLEL_ALGO
#define PARALLEL_ALGO

#include <algorithm>
#include <future>
#include <vector>
#include <list>

namespace Parallel {

template <typename T> int Partition(std::vector<T> &arr, int start, int end) {
  std::swap(arr[end], arr[start + rand() % (end - start)]);
  T const &pivot = arr[end];
  int j = start - 1;
  for (int i = start; i <= end; i++) {
    if (arr[i] < pivot) {
      std::swap(arr[++j], arr[i]);
    }
  }
  std::swap(arr[j + 1], arr[end]);
  return j + 1;
}

template <typename T> void SortVector(std::vector<T> &arr, int start, int end) {
  if (end - start < 1)
    return;
  int pivot_idx = Partition(arr, start, end);
  auto handle1 = std::async(&SortVector<T>, std::ref(arr), start, pivot_idx - 1);
  auto handle2 = std::async(&SortVector<T>, std::ref(arr), pivot_idx + 1, end);
}

template <typename T>
std::list<T> SortList(std::list<T> arr) {
    if (arr.empty()) return arr;
    std::list<T> sorted;
    sorted.splice(sorted.begin(), arr, arr.begin());
    T const& pivot = *sorted.begin();
    auto pivot_it = std::partition(arr.begin(), arr.end(), [&] (T const& t) {return t < pivot;});
    std::list<T> left;
    left.splice(left.end(), arr, arr.begin(), pivot_it);
    std::future<std::list<T>> sorted_left = std::async(&SortList<T>, std::move(left));
    std::future<std::list<T>> sorted_right = std::async(&SortList<T>, std::move(arr));
    sorted.splice(sorted.end(), sorted_right.get());
    sorted.splice(sorted.begin(), sorted_left.get());
    return sorted;
}


} // namespace Parallel

#endif