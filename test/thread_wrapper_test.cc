#include "src/thread_wrapper.h"
#include <functional>

#include "gtest/gtest.h"

namespace Thread{

TEST(RAIIThreadTest, BasicJoin) {
    auto f = [] ()-> void {};
    RAIIThread t(std::thread(f), RAIIThread::DtorAction::Join);
}

TEST(RAIIThreadTest, BasicDetach) {
    auto f = [] ()-> void {};
    RAIIThread t(std::thread(f), RAIIThread::DtorAction::Detach);
}

TEST(RealAsyncTest, Basic) {
    auto f = [] ()-> int { return 1; };
    std::future<int> fut = realAsync(f);
    EXPECT_EQ(fut.get(), 1);
}


}  // namespace
