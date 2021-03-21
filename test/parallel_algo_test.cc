#include "src/parallel_algo.h"
#include "gtest/gtest.h"

namespace Parallel {
    TEST(SortListTest, Basic) {
        std::list<int> arr{31, 23, 5, 5, 7, 44, 1};
        SortList<int>(arr);
    }
};