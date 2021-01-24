#include "pch.h"

#include "mul/subscription.h"

using namespace ::testing;

namespace
{

class handler_mock
{
public:
    MOCK_METHOD(void, fn, (), ());
};

} // anonymous namespace

TEST(subscription, construct_destruct) {
    StrictMock<handler_mock> handler;

    {
        mul::subscription subscription([&handler]() { handler.fn(); });

        EXPECT_CALL(handler, fn());
    }
}

TEST(subscription, no_unsubscriber) {
    mul::subscription subscription;
}

TEST(subscription, null_unsubscriber) {
    mul::subscription subscription(nullptr);
}

TEST(subscription, moveable) {
    StrictMock<handler_mock> handler1;
    StrictMock<handler_mock> handler2;

    mul::subscription subscription1([&handler1]() { handler1.fn(); });

    {
        mul::subscription subscription2([&handler2]() { handler2.fn(); });

        EXPECT_CALL(handler1, fn());

        subscription1 = std::move(subscription2);
    }

    EXPECT_CALL(handler2, fn());
}
