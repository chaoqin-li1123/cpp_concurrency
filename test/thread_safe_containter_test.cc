
#include "src/thread_pool.h"
#include "gtest/gtest.h"

namespace Container
{

    class ThreadSafeQueueTest : public testing::Test
    {
    protected:
        ThreadSafeQueue<int> q_;
    };

    TEST_F(ThreadSafeQueueTest, PushNPopN)
    {
        int N = 100;
        for (int i = 0; i < N; i++)
            q_.push(i);
        int temp;
        for (int i = 0; i < N; i++)
        {
            EXPECT_TRUE(q_.try_pop(temp));
        }
        EXPECT_TRUE(q_.empty());
    }

    TEST_F(ThreadSafeQueueTest, FirstInFirstOut)
    {
        int N = 100;
        for (int i = 0; i < N; i++)
        {
            q_.push(i);
        }
        int temp;
        for (int i = 0; i < N; i++)
        {
            EXPECT_TRUE(q_.try_pop(temp));
            EXPECT_EQ(temp, i);
        }
        EXPECT_TRUE(q_.empty());
    }

    TEST_F(ThreadSafeQueueTest, PushNMultiThread)
    {
        Thread::ThreadPoolImpl pool;
        pool.start();
        const int N = 100;
        for (int i = 0; i < N; i++)
        {
            pool.addTask(
                [&, i]() { q_.push(i); });
        }
        pool.shutdown();
        int temp;
        for (int i = 0; i < N; i++)
        {
            EXPECT_TRUE(q_.try_pop(temp));
        }
        EXPECT_TRUE(q_.empty());
    }


    class ThreadSafeStackTest : public testing::Test
    {
    public:
        ThreadSafeStack<int> stk_;
    };

    TEST_F(ThreadSafeStackTest, PushNPopN) {
        int N = 100;
        for (int i = 0; i < N; i++) {
            stk_.push(i);
        }
        int temp;
        for (int i = 0; i < N; i++)
        {
            EXPECT_TRUE(stk_.try_pop(temp));
        }
        EXPECT_TRUE(stk_.empty());
    }

    TEST_F(ThreadSafeStackTest, FirstInLastOut)
    {
        int N = 100;
        for (int i = 0; i < N; i++)
        {
            stk_.push(i);
        }
        int temp;
        for (int i = N - 1; i >= 0; i--)
        {
            EXPECT_TRUE(stk_.try_pop(temp));
            EXPECT_EQ(temp, i);
        }
        EXPECT_TRUE(stk_.empty());
    }

    TEST_F(ThreadSafeStackTest, PushNMultiThread)
    {
        Thread::ThreadPoolImpl pool;
        pool.start();
        const int N = 100;
        for (int i = 0; i < N; i++)
        {
            pool.addTask(
                [&, i]() { stk_.push(i); });
        }
        pool.shutdown();
        EXPECT_EQ(stk_.size(), N);
        int temp;
        for (int i = 0; i < N; i++)
        {
            EXPECT_TRUE(stk_.try_pop(temp));
        }
        EXPECT_TRUE(stk_.empty());
    }

} // namespace
