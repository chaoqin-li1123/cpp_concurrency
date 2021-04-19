#ifndef THREAD_WRAPPER
#define THREAD_WRAPPER

#include <thread>
#include <future>

namespace Thread {
    class RAIIThread {
    public:
        enum class DtorAction { Join, Detach };
        RAIIThread(std::thread&& t): t_{std::move(t)}, dtor_action_{DtorAction::Join} {}
        RAIIThread(std::thread&& t, DtorAction dtor_action): t_{std::move(t)}, dtor_action_{dtor_action} {}
        ~RAIIThread() {
            if (dtor_action_ == DtorAction::Join) {
                t_.join();
            }
            else {
                t_.detach();
            }
        }
    private:
        std::thread t_;
        DtorAction dtor_action_;
    };

    template <typename Callable, typename... Args> 
    std::future<typename std::result_of<Callable(Args...)>::type>
    realAsync(Callable&& callable, Args&&... args) {
        return std::async(std::launch::async, 
                std::forward<Callable>(callable), std::forward<Args>(args)...);
    }

};


#endif
