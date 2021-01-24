#include "pch.h"

#include "mul/event.h"

using namespace ::testing;

namespace
{

class handler_mock
{
public:
    MOCK_METHOD(void, exception, (const std::exception&), ());
    MOCK_METHOD(void, nr, (int), ());
    MOCK_METHOD(void, nr_and_string, (int, std::string), ());
};

} // anonymous namespace

TEST(event, subscribe_unsubscribe) {
    mul::event<int, std::string> event;

    StrictMock<handler_mock> handler;

    {
        auto subscription = event.subscribe([&handler](int nr, std::string str) { handler.nr_and_string(nr, str); });

        EXPECT_CALL(handler, nr_and_string(5, "Test String"));

        event(5, "Test String");
    }

    event(5, "Test String");
}

TEST(event, subscribe_twice) {
    mul::event<int, std::string> event;

    StrictMock<handler_mock> handler1;
    StrictMock<handler_mock> handler2;

    auto subscription = event.subscribe([&handler1](int nr, std::string str) { handler1.nr_and_string(nr, str); });
    {
        auto subscription = event.subscribe([&handler2](int nr, std::string str) { handler2.nr_and_string(nr, str); });

        InSequence handlers_must_be_called_in_order_of_subscription;
        EXPECT_CALL(handler1, nr_and_string(42, "both called"));
        EXPECT_CALL(handler2, nr_and_string(42, "both called"));

        event(42, "both called");
    }

    EXPECT_CALL(handler1, nr_and_string(43, "only handler1 called"));

    event(43, "only handler1 called");
}

TEST(event, handler_throws_calls_exception_handler) {
    StrictMock<handler_mock> handler;

    mul::event<int> event([&handler](const std::exception& e) { handler.exception(e); });

    auto subscription = event.subscribe([&handler](int nr) { handler.nr(nr); });

    EXPECT_CALL(handler, nr(4)).WillOnce(Throw(std::runtime_error("expected exception")));
    EXPECT_CALL(handler, exception(_)).WillOnce([](const std::exception& e)
    {
        EXPECT_THAT(dynamic_cast<const std::runtime_error*>(&e), NotNull());
        EXPECT_THAT(e.what(), StrEq("expected exception"));
    });

    EXPECT_NO_THROW(event(4));
}

TEST(event, handler_throws_other_handlers_still_called) {
    mul::event<int, std::string> event;

    StrictMock<handler_mock> handler1;
    StrictMock<handler_mock> handler2;

    auto subscription1 = event.subscribe([&handler1](int nr, std::string str) { handler1.nr_and_string(nr, str); });
    auto subscription2 = event.subscribe([&handler2](int nr, std::string str) { handler2.nr_and_string(nr, str); });

    EXPECT_CALL(handler1, nr_and_string(1, "test")).WillOnce(Throw(std::runtime_error("expected exception")));
    EXPECT_CALL(handler2, nr_and_string(1, "test"));

    EXPECT_NO_THROW(event(1, "test"));
}

TEST(event, handler_throws_non_std_exception) {
    StrictMock<handler_mock> handler;

    mul::event<int> event([&handler](const std::exception& e) { handler.exception(e); });;

    auto subscription = event.subscribe([&handler](int nr) { handler.nr(nr); });

    EXPECT_CALL(handler, nr(5)).WillOnce(Throw("not an std::exception"));
    EXPECT_CALL(handler, exception(_)).Times(0);

    EXPECT_NO_THROW(event(5));
}

TEST(event, exception_handler_throws) {
    StrictMock<handler_mock> handler;

    mul::event<int> event([&handler](const std::exception& e) { handler.exception(e); });;

    auto subscription = event.subscribe([&handler](int nr) { handler.nr(nr); });

    EXPECT_CALL(handler, nr(6)).WillOnce(Throw(std::runtime_error("expected exception")));
    EXPECT_CALL(handler, exception(_)).WillOnce(Throw(std::logic_error("exception handler throws")));

    EXPECT_THROW(event(6), std::logic_error);
}
