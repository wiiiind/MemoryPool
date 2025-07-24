#include "../include/MemoryPool.h"
#include "../include/Common.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <numeric> // for std::accumulate

// 为 MemoryPool 类创建一个明确的别名以解决命名冲突
using MemPool = MemoryPool::MemoryPool;

// 定义一个简单的计时器
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    // 返回耗时，单位毫秒(ms)
    double elapsed() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// --- 基准测试函数 ---
// n_threads: 线程数
// n_allocs: 每个线程的分配次数
// block_size: 每次分配的块大小
void benchmark(const char* name, bool use_mem_pool, size_t n_threads, size_t n_allocs, size_t block_size) {
    std::cout << "Benchmarking: " << name << " (" << n_threads << " threads, " << n_allocs << " allocs/thread, " << block_size << " bytes)..." << std::endl;

    // 用于收集每个线程的耗时
    std::vector<double> thread_times(n_threads);
    
    auto thread_func = [&](size_t thread_idx) {
        std::vector<void*> ptrs;
        ptrs.reserve(n_allocs);
        
        Timer t;
        if (use_mem_pool) {
            for (size_t i = 0; i < n_allocs; ++i) {
                ptrs.push_back(MemPool::allocate(block_size));
            }
            for (size_t i = 0; i < n_allocs; ++i) {
                MemPool::deallocate(ptrs[i], block_size);
            }
        } else {
            for (size_t i = 0; i < n_allocs; ++i) {
                ptrs.push_back(new char[block_size]);
            }
            for (size_t i = 0; i < n_allocs; ++i) {
                delete[] static_cast<char*>(ptrs[i]);
            }
        }
        thread_times[thread_idx] = t.elapsed();
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < n_threads; ++i) {
        threads.emplace_back(thread_func, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    double total_time = std::accumulate(thread_times.begin(), thread_times.end(), 0.0);
    printf("  -> Total time: %.2f ms\n\n", total_time);
}

int main() {
    const size_t ALLOCS = 100000;

    std::cout << "=======================================" << std::endl;
    std::cout << "    STARTING PERFORMANCE BENCHMARKS" << std::endl;
    std::cout << "=======================================" << std::endl;

    // --- 场景1: 单线程，小对象分配 ---
    benchmark("MemoryPool (1T, small obj)", true, 1, ALLOCS, 16);
    benchmark("new/delete (1T, small obj)", false, 1, ALLOCS, 16);

    // --- 场景2: 多线程，小对象分配 ---
    // 这是最能体现 ThreadCache 优势的场景
    benchmark("MemoryPool (8T, small obj)", true, 8, ALLOCS, 32);
    benchmark("new/delete (8T, small obj)", false, 8, ALLOCS, 32);

    // --- 场景3: 多线程，中等对象分配 ---
    // 这个场景下，ThreadCache 和 CentralCache 会频繁交互
    benchmark("MemoryPool (8T, medium obj)", true, 8, ALLOCS, 256);
    benchmark("new/delete (8T, medium obj)", false, 8, ALLOCS, 256);
    
    // --- 场景4: 多线程，大对象分配 ---
    // 这个场景下，双方都应该直接走系统调用，性能差距不大
    // MAX_BYTES 定义在 MemoryPool 命名空间中
    benchmark("MemoryPool (4T, large obj)", true, 4, ALLOCS/10, MemoryPool::MAX_BYTES + 1);
    benchmark("new/delete (4T, large obj)", false, 4, ALLOCS/10, MemoryPool::MAX_BYTES + 1);
    
    std::cout << "=======================================" << std::endl;
    std::cout << "     PERFORMANCE BENCHMARKS FINISHED" << std::endl;
    std::cout << "=======================================" << std::endl;

    return 0;
}