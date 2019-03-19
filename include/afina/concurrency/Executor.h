#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <atomic>

namespace Afina {
namespace Concurrency {

class Executor;
void perform (Executor *executor);

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

    Executor () {};
    ~Executor() {};

    Executor (unsigned int low_watermark, unsigned int high_watermark, unsigned int max_queue_size, size_t idle_time);

    /**
     * Start _low_watermark threads and wait for jobs
     */
    void Start ();

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false);

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {

        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(mutex);
        if (state != State::kRun) {
            return false;
        }

        // Enqueue new task
        if (tasks.size() >= _max_queue_size) {
            return false;
        }
        tasks.push_back(exec);

        // Release work to threads
        if (_n_free_workers == 0 && _n_existing_workers == _high_watermark) {
            return false;
        }

        if (_n_free_workers == 0) {
            _n_existing_workers++;
            std::thread tmp (perform, this);
            tmp.detach ();
        }

        empty_condition.notify_one();
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor);

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;
    std::condition_variable stop_condition;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state = State::kStopped;

    unsigned int _low_watermark = 0;
    unsigned int _high_watermark = 0;
    unsigned int _max_queue_size = 0;
    size_t _idle_time = 0;

    unsigned int _n_existing_workers = 0;
    unsigned int _n_free_workers = 0;
};

} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
