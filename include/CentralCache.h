#pragma once
#include "Common.h"
#include <array>
#include <atomic>
#include <mutex>

namespace MemoryPool {
    class CentralCache {
    public:
        static CentralCache& getInstance() {
            static CentralCache instance;
            return instance;
        }

        // 申请内存，index是内存块大小类别的索引，期望获得batchNum个内存块，
        // 返回值是内存块链表的头指针
        void* fetchRange(size_t index, size_t batchNum);

        // 归还内存，（头指针，大小，索引）
        void returnRange(void* start, size_t size, size_t index);

        private:
        CentralCache();
        CentralCache(const CentralCache&) = delete;
        CentralCache& operator=(const CentralCache&) = delete;

        // 申请Span
        void* fetchFromPageCache(size_t index);

        private:
        // 空闲链表的头指针，原子化（atomic）指针保证多线程操作的安全
        std::array<std::atomic<void*>, FREE_LIST_SIZE> centralFreeList;

        // 为每个链表准备一个自旋锁，atomic_flag是最轻量的原子类型
        std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;

    };
}