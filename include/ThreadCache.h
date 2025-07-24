#pragma once
#include "Common.h"
#include <array>

namespace MemoryPool
{

    // 思想：ThreadCache 是每个线程独有的，因此它的所有操作都是无锁的。
    class ThreadCache
    {
    public:
        // 思想：使用 thread_local 和单例模式结合。
        // `static thread_local ThreadCache instance;` 这行代码是关键。
        // 当一个线程第一次调用 getInstance 时，它会创建一个只属于自己的 instance。
        // 后续调用将返回同一个 instance。不同线程间的 instance 是完全隔离的。
        // thread_local的意思是，这个变量的生命周期和线程生命周期一样
        static ThreadCache* getInstance()
        {
            static thread_local ThreadCache instance;
            return &instance;
        }

        // 公开接口1：分配内存
        void* allocate(size_t size);

        // 公开接口2：释放内存
        void deallocate(void* ptr, size_t size);

    private:
        ThreadCache()
        {
            // 初始化自由链表和大小统计
            freeList_.fill(nullptr);
            // freeListSize_ 在 C++ 中会被默认零初始化，无需手动处理
        }
        ThreadCache(const ThreadCache&) = delete;
        ThreadCache& operator=(const ThreadCache&) = delete;

        // 私有辅助函数1：当自由链表为空时，向 CentralCache 获取内存。
        // 注意，它返回的是一个内存块，而不是像 CentralCache 那样返回一个链表。
        void* fetchFromCentralCache(size_t index);

        // 私有辅助函数2：当自由链表过长时，将部分内存归还给 CentralCache。
        void returnToCentralCache(size_t index);

    private:
        // 核心数据结构1：线程本地的自由链表数组。
        // 数组的每个元素都是一个链表的头指针。
        // 因为是线程本地的，所以不需要 std::atomic，普通的指针就足够了。
        std::array<void*, FREE_LIST_SIZE>  freeList_;

        // 核心数据结构2：用于统计每个自由链表长度。
        // 这个计数的目的是为了触发 returnToCentralCache。
        std::array<size_t, FREE_LIST_SIZE> freeListSize_;

    };

} // namespace MemoryPool