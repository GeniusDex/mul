#pragma once

#include <exception>
#include <functional>
#include <list>

#include "subscription.h"

namespace mul
{

template<typename... argument_types>
class event
{
public:
    // The function type of handler functions for this event.
    using handler_type = std::function<void(argument_types...)>;

    // Construct an event with an optional exception handler.
    explicit event(const std::function<void(const std::exception&)>& exception_handler = nullptr)
        : m_exception_handler(exception_handler)
    {}

    // Moving an event is not possible since the subscriptions include a this pointer which
    // would then be invalidated.
    event(event&&) = delete;
    event& operator=(event&&) = delete;

    // An event is not copyable since the event handler subscriptions are linked to one specific
    // event object. Allowing copies of an event object to exist would break reliable unsubscriptions.
    event(const event&) = delete;
    event& operator=(const event&) = delete;

    // Subscribe the given handler to this event for the lifetime of the returned subscription.
    //
    // To unsubscribe from this event, ensure the subscription gets destructed.
    subscription subscribe(const handler_type& handler)
    {
        auto it = m_handlers.insert(m_handlers.end(), handler);

        return subscription( [this, it]() { unsubscribe(it); } );
    }

    // Call all subscribed event handlers.
    //
    // If any event handler throws an exception which derives from std::exception, the registered
    // exception handler is called. If this exception handler itself throws, further processing
    // of the event is cancelled. This is typically not desired.
    //
    // If any event handler throws an exception which does not derive from std::exception, the
    // exception is ignored and the next handler will be called as normal.
    void operator()(argument_types... args)
    {
        for (const auto& handler : m_handlers)
        {
            try
            {
                handler(args...);
            }
            catch (const std::exception& e)
            {
                if (m_exception_handler)
                {
                    m_exception_handler(e);
                }
            }
            catch (...)
            {
                // unknown exceptions are ignored
            }
        }
    }

private:
    void unsubscribe(const typename std::list<handler_type>::const_iterator& handler_it)
    {
        m_handlers.erase(handler_it);
    }

private:
    std::list<handler_type> m_handlers;
    std::function<void(const std::exception&)> m_exception_handler;
};

} // namespace mul
