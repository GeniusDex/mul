#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace mul
{

class worker_thread
{
public:
    worker_thread(const std::function<void(const std::exception&)>& exception_handler = nullptr)
        : m_exception_handler(exception_handler)
        , m_thread([this]() { run(); })
    {}

    ~worker_thread()
    {
        trigger_shutdown();

        m_thread.join();
    }

    // Schedule a function to be executed on the worker thread.
    void enqueue(const std::function<void()>& work_fn)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_queue.push(work_fn);
        m_cv.notify_one();
    }

    // Execute a function on the worker thread and wait for its result.
    template<typename fn_type>
    auto perform(const fn_type& work_fn) -> decltype(work_fn())
    {
        using result_type = decltype(work_fn());

        std::packaged_task<result_type()> task(work_fn);
        auto future = task.get_future();

        enqueue([&task] { task(); });

        return future.get();
    }

private:
    void run()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_shutdown)
        {
            while (!m_queue.empty())
            {
                auto work_fn = m_queue.front();
                m_queue.pop();

                lock.unlock();

                call_and_handle_exceptions(work_fn);

                lock.lock();
            }

            m_cv.wait(lock);
        }
    }

    void call_and_handle_exceptions(const std::function<void()>& fn)
    try
    {
        fn();
    }
    catch (std::exception& e)
    {
        m_exception_handler(e);
    }
    catch (...)
    {
        // unknown exceptions are ignored
    }

    void trigger_shutdown()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_shutdown = true;
        m_cv.notify_one();
    }

private:
    std::function<void(const std::exception&)> m_exception_handler;
    std::mutex m_mutex; // guards access to m_queue and m_shutdown
    std::condition_variable m_cv; // indicates changes to m_queue or m_shutdown
    std::queue<std::function<void()>> m_queue;
    bool m_shutdown = false;
    std::thread m_thread;
};

} // namespace mul
