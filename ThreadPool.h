/**
* YATO library
*
* Apache License, Version 2.0
* Copyright (c) 2016-2020 Alexey Gruzdev
*/

#ifndef _YATO_ACTORS_THREAD_POOL_H_
#define _YATO_ACTORS_THREAD_POOL_H_

#include <condition_variable>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * Simplest thread pool.
 */
class ThreadPool
{
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;

    mutable std::mutex m_mutex;
    std::condition_variable m_cvar;

    bool m_stop = false;

    std::mutex m_loop_mutex;
    std::condition_variable m_loop_cv;
    mutable std::condition_variable m_wait_cv;
    std::atomic_uint32_t m_running{};

//        logger_ptr m_log;

public:
    ThreadPool(size_t threads_num) {
        auto thread_function = [this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cvar.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                    if (m_stop && m_tasks.empty()) {
                        break;
                    }
                    if (!m_tasks.empty()) {
                        m_running.fetch_add(1, std::memory_order_release);
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                }
                // Execute
                if (task) {
                    try {
                        task();
                    }
                    catch (std::exception & /*e*/) {
                        //m_log->error("yato::actors::thread_pool[thread_function]: Unhandled exception: %s", e.what());
                    }
                    catch (...) {
                        //m_log->error("yato::actors::thread_pool[thread_function]: Unhandled exception.");
                    }
                }
                m_running.fetch_sub(1, std::memory_order_release);
                m_wait_cv.notify_all();
            }
        };

        for(size_t i = 0; i < threads_num; ++i) {
            m_threads.emplace_back(thread_function);
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cvar.notify_all();
        for(auto & thread : m_threads) {
            thread.join();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Add task
    template <typename FunTy_, typename ... Args_>
    void Enqueue(FunTy_ && function, Args_ && ... args) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if(!m_stop) {
            m_tasks.emplace(std::bind(std::forward<FunTy_>(function), std::forward<Args_>(args)...));
        } else {
            //m_log->warning("yato::actor::thread_pool[enqueue]: Failed to enqueue a new task. Pool is already stopped.");
        }
        m_cvar.notify_one();
    }

    void wait() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wait_cv.wait(lock, [this]() -> bool { return 0 == m_running.load(std::memory_order_acquire) && m_tasks.empty(); });
    }
};

#endif //_YATO_ACTORS_THREAD_POOL_H_
