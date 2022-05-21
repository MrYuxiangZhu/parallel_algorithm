#pragma once

#include <thread>
#include <vector>
#include <numeric>
#include <future>
#include <functional>

class thread_guard
{
public:
    explicit thread_guard(std::vector<std::thread>& _threads) : threads(_threads) {  }
    ~thread_guard()
    {
        for (auto& thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }
private:
    std::vector<std::thread>& threads;
};

template <typename Iterator, typename Type, typename Func>
struct accumulate_block
{
    Type operator() (Iterator first, Iterator last, Type init, Func func)
    {
        return std::accumulate(first, last, init, func);
    }
};

template <typename Iterator, typename Type, typename Func>
Type parallel_accumulate(Iterator first, Iterator last, Type init, Func func)
{
    const unsigned long length = static_cast<unsigned long>(std::distance(first, last));

    if (!length)
    {
        return init;
    }

    const unsigned long min_per_thread = 12;
    const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware_threads = std::thread::hardware_concurrency();
    const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    const unsigned long block_size = length / num_threads;

    std::vector<std::future<Type>> futures(num_threads - 1);
    std::vector<std::thread> threads(num_threads - 1);

    thread_guard guard(threads);

    Iterator block_start = first;
    for (unsigned long i = 0; i < num_threads - 1; ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<Type(Iterator, Iterator, Type, Func)> task([=](Iterator first, Iterator last, Type init, Func func) { return accumulate_block<Iterator, Type, Func>()(first, last, init, func); });
        futures[i] = task.get_future();
        threads[i] = std::thread(std::move(task), block_start, block_end, init, func);
        block_start = block_end;
    }

    Type last_result = accumulate_block<Iterator, Type, Func>()(block_start, last, init, func);

    for (auto& iter : futures)
    {
        last_result = func(iter.get(), last_result);
    }

    return last_result;
}

template <typename Iterator, typename Type>
Type parallel_plus(Iterator first, Iterator last, Type init)
{
    Type result = parallel_accumulate(first, last, Type(0), std::plus<Type>{ });
    return result + init;
}

template <typename Iterator, typename Type>
Type parallel_minus(Iterator first, Iterator last, Type init)
{
    Type result = parallel_accumulate(first, last, Type(0), std::plus<Type>{ });
    return init - result;
}

template <typename Iterator, typename Type>
Type parallel_multiplies(Iterator first, Iterator last, Type init)
{
    Type result = parallel_accumulate(first, last, Type(1), std::multiplies<Type>{ });
    return result * init;
}

