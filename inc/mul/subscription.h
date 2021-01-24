#pragma once

#include <functional>

namespace mul
{

class subscription
{
public:
    explicit subscription(const std::function<void()>& unsubscriber = nullptr)
        : m_unsubscriber(unsubscriber)
    {}

    subscription(subscription&&) = default;
    subscription& operator=(subscription&& other) noexcept
    {
        if (m_unsubscriber)
        {
            m_unsubscriber();
        }

        m_unsubscriber = std::move(other.m_unsubscriber);

        return *this;
    }

    subscription(const subscription&) = delete;
    subscription& operator=(const subscription&) = delete;

    ~subscription()
    {
        if (m_unsubscriber)
        {
            m_unsubscriber();
        }
    }

private:
    std::function<void()> m_unsubscriber;
};

} // namespace mul
