#include "src/thread_pool.h"
#include "gtest/gtest.h"

namespace Thread {
    class ThreadPoolTest : public testing::Test{
    public:
        ThreadPoolImpl thread_pool_;
    };

    TEST_F(ThreadPoolTest, MultiThreadIncrement)
    {
        thread_pool_.start();
        std::atomic<int> cnt{0};
        const int N = 100;
        for (int i = 0; i < N; i++)
        {
            thread_pool_.addTask(
                [&cnt]() { cnt++; });
        }
        thread_pool_.shutdown();
        EXPECT_EQ(cnt.load(), N);
    }
};