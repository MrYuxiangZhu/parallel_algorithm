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

template <typename Iterator, typename Type>
struct accumulate_block
{
    Type operator() (Iterator first, Iterator last)
    {
        return std::accumulate(first, last, Type());
    }
};

template <typename Iterator, typename Type>
Type parallel_accumulate_1(Iterator first, Iterator last, Type init)
{
    const unsigned long length = (unsigned long)std::distance(first, last);

    if (!length)
    {
        return init;
    }

    const unsigned long min_per_thread = 25;
    const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware_threads = std::thread::hardware_concurrency();
    const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    const unsigned long block_size = length / num_threads;

    std::vector<std::future<Type>> futures(num_threads - 1);
    std::vector<std::thread> threads(num_threads - 1);
    
    Iterator block_start = first;
    for (unsigned long i = 0; i < num_threads - 1; ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<Type(Iterator, Iterator)> task([](Iterator first, Iterator last) { return accumulate_block<Iterator, Type>()(first, last); });
        futures[i] = task.get_future();
        threads[i] = std::thread(std::move(task), block_start, block_end);
        block_start = block_end;
    }

    Type last_result = accumulate_block<Iterator, Type>()(block_start, last);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

    Type result = init;

    for (auto& iter : futures)
    {
        result += iter.get();
    }

    result += last_result;
    return result;
}

template <typename Iterator, typename Type>
Type parallel_accumulate_2(Iterator first, Iterator last, Type init)
{
    const unsigned long length = (unsigned long)std::distance(first, last);

    if (!length)
    {
        return init;
    }

    const unsigned long min_per_thread = 25;
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
        std::packaged_task<Type(Iterator, Iterator)> task([](Iterator first, Iterator last) { return accumulate_block<Iterator, Type>()(first, last); });
        futures[i] = task.get_future();
        threads[i] = std::thread(std::move(task), block_start, block_end);
        block_start = block_end;
    }

    Type last_result = accumulate_block<Iterator, Type>()(block_start, last);
    //std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

    Type result = init;

    for (auto& iter : futures)
    {
        result += iter.get();
    }

    result += last_result;
    return result;
}