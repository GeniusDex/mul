#include "pch.h"

#include "mul/worker_thread.h"

#include <condition_variable>
#include <mutex>
#include <thread>

using namespace ::testing;

namespace
{

class handler_mock
{
public:
    MOCK_METHOD(void, exception, (const std::exception&), ());
};

} // anonymous namespace

TEST(worker_thread, perform) {
    mul::worker_thread worker;

    auto test_thread_id = std::this_thread::get_id();
    auto worker_thread_id = worker.perform([] { return std::this_thread::get_id(); });
    EXPECT_THAT(worker_thread_id, Ne(test_thread_id));
}

TEST(worker_thread, perform_throws) {
    StrictMock<handler_mock> handler;
    mul::worker_thread worker([&](const std::exception& e) { handler.exception(e); });

    EXPECT_CALL(handler, exception(_)).Times(0);

    EXPECT_THROW(worker.perform([] { throw std::runtime_error("expected exception"); }), std::runtime_error);
}

TEST(worker_thread, enqueue) {
    mul::worker_thread worker;

    std::mutex mutex;
    std::condition_variable cv;
    bool waiting = false;
    std::promise<int> promise;
    auto future = promise.get_future();

    worker.enqueue([&]
        {
            cv.notify_one();
            waiting = true;
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock);
            promise.set_value(42);
        });

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return waiting; });
    }

    cv.notify_one();

    EXPECT_THAT(future.get(), Eq(42));
}

TEST(worker_thread, enqueue_throws) {
    StrictMock<handler_mock> handler;
    mul::worker_thread worker([&](const std::exception& e) { handler.exception(e); });

    EXPECT_CALL(handler, exception(_)).WillOnce([](const std::exception& e)
        {
            EXPECT_THAT(dynamic_cast<const std::runtime_error*>(&e), NotNull());
            EXPECT_THAT(e.what(), StrEq("expected exception"));
        });

    EXPECT_NO_THROW(worker.enqueue([] { throw std::runtime_error("expected exception"); }));

    worker.perform([] {});
}

TEST(worker_thread, enqueue_throws_non_std_exception) {
    StrictMock<handler_mock> handler;
    mul::worker_thread worker([&](const std::exception& e) { handler.exception(e); });

    EXPECT_CALL(handler, exception(_)).Times(0);

    EXPECT_NO_THROW(worker.enqueue([] { throw "not an std::exception"; }));

    worker.perform([] {});
}
