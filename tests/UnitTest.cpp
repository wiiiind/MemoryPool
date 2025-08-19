#include "../include/MemoryPool.h"
#include "../include/Common.h"
#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <atomic>
#include <algorithm> // for std::shuffle
#include <random>    // for std::mt19937

// 为 MemoryPool 类创建一个明确的别名以解决命名冲突
using MemPool = MemoryPool::MemoryPool;

// 测试1：基础分配与释放
void testBasicAllocation() {
    std::cout << "Running: Basic Allocation Test..." << std::endl;

    // 测试一个小对象
    void* p1 = MemPool::allocate(8);
    assert(p1 != nullptr);
    std::cout << "  Allocated 8 bytes at: " << p1 << std::endl;
    MemPool::deallocate(p1, 8);

    // 测试一个中等对象
    void* p2 = MemPool::allocate(1024);
    assert(p2 != nullptr);
    std::cout << "  Allocated 1024 bytes at: " << p2 << std::endl;
    MemPool::deallocate(p2, 1024);

    // 测试一个大对象 (将直接走 malloc/free)
    // MAX_BYTES 定义在 MemoryPool 命名空间中
    void* p3 = MemPool::allocate(MemoryPool::MAX_BYTES + 1);
    assert(p3 != nullptr);
    std::cout << "  Allocated large object (" << MemoryPool::MAX_BYTES + 1 << " bytes) at: " << p3 << std::endl;
    MemPool::deallocate(p3, MemoryPool::MAX_BYTES + 1);

    std::cout << "Basic Allocation Test PASSED!" << std::endl;
}

// 测试2：内存写入与内容验证
void testMemoryWriting() {
    std::cout << "Running: Memory Writing Test..." << std::endl;
    const size_t size = 128;
    char* ptr = static_cast<char*>(MemPool::allocate(size));
    assert(ptr != nullptr);

    // 写入数据
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = static_cast<char>(i % 128);
    }
    // 验证数据
    for (size_t i = 0; i < size; ++i) {
        assert(ptr[i] == static_cast<char>(i % 128));
    }
    MemPool::deallocate(ptr, size);
    std::cout << "Memory Writing Test PASSED!" << std::endl;
}

// 测试3：多线程压力测试
void testMultiThreading() {
    std::cout << "Running: Multi-Threading Stress Test..." << std::endl;
    const int NUM_THREADS = 8;
    const int ALLOCS_PER_THREAD = 5000;
    std::atomic<bool> has_error{false};

    auto thread_func = [&](int seed) {
        std::vector<std::pair<void*, size_t>> allocations;
        allocations.reserve(ALLOCS_PER_THREAD);
        std::mt19937 gen(seed); // 每个线程使用不同的随机种子

        for (int i = 0; i < ALLOCS_PER_THREAD; ++i) {
            // MAX_BYTES 定义在 MemoryPool 命名空间中
            size_t size = (gen() % (MemoryPool::MAX_BYTES / 8)) + 1; // 随机分配各种大小
            void* ptr = MemPool::allocate(size);
            if (!ptr) {
                std::cerr << "Thread " << std::this_thread::get_id() << " failed to allocate " << size << " bytes." << std::endl;
                has_error = true;
                break;
            }
            allocations.push_back({ptr, size});
        }

        // 随机打乱后释放，模拟真实的乱序释放场景
        std::shuffle(allocations.begin(), allocations.end(), gen);

        for (const auto& alloc : allocations) {
            MemPool::deallocate(alloc.first, alloc.second);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(thread_func, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    assert(!has_error);
    std::cout << "Multi-Threading Stress Test PASSED!" << std::endl;
}


int main() {
    std::cout << "=======================================" << std::endl;
    std::cout << "      STARTING UNIT TESTS" << std::endl;
    std::cout << "=======================================" << std::endl;

    testBasicAllocation();
    std::cout << "---------------------------------------" << std::endl;
    testMemoryWriting();
    std::cout << "---------------------------------------" << std::endl;
    testMultiThreading();

    std::cout << "=======================================" << std::endl;
    std::cout << "      ALL UNIT TESTS PASSED!" << std::endl;
    std::cout << "=======================================" << std::endl;

    return 0;
}