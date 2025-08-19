#pragma once
#include "ThreadCache.h"

namespace MemoryPool
{

    // 思想：外观模式（Facade Pattern）。
    // MemoryPool 类本身不实现任何复杂逻辑，它只是一个简单的门面，
    // 将用户的请求转发给底层的 ThreadCache。
    // 这让用户的使用变得极其简单，无需关心三层架构的复杂性。
    class MemoryPool
    {
    public:
        static void* allocate(size_t size)
        {
            // 所有分配请求都交给当前线程的 ThreadCache 实例处理
            return ThreadCache::getInstance()->allocate(size);
        }

        static void deallocate(void* ptr, size_t size)
        {
            // 所有释放请求也交给当前线程的 ThreadCache 实例处理
            ThreadCache::getInstance()->deallocate(ptr, size);
        }
    };

} // namespace MemoryPool