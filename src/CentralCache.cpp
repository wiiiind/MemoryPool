#include "../include/CentralCache.h"
#include "../include/PageCache.h"
#include <thread>
#include <cassert>

namespace MemoryPool {
    CentralCache::CentralCache() {
        for (auto& ptr: centralFreeList) {
            ptr.store(nullptr, std::memory_order_relaxed);
        }
        for (auto& lock: locks_) {
            lock.clear();
        }
    }

    // 最终重构版：CentralCache::fetchFromPageCache
    // 它的新职责是：补货，并返回补货后得到的完整的新链表的头指针。
    void* CentralCache::fetchFromPageCache(size_t index)
    {
        // 1. 计算所需内存块的实际大小
        size_t blockSize = (index + 1) * ALIGNMENT;

        // 2. 动态决定一次性从 PageCache 申请多少页 (这部分逻辑不变)
        const size_t SPAN_PAGES = 8;
        size_t numPages = 0;
        if (blockSize <= SPAN_PAGES * PageCache::PAGE_SIZE) {
            numPages = SPAN_PAGES;
        } else {
            numPages = (blockSize + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
        }
        if (numPages == 0) numPages = 1;

        // 3. 向 PageCache 申请 Span
        void* spanStart = PageCache::getInstance().allocateSpan(numPages);
        if (!spanStart) {
            return nullptr;
        }

        // 4. [核心改进] 将获取到的整个 Span，全部切割成 blockSize 大小的内存块，并串成一个完整的链表
        char* current = static_cast<char*>(spanStart);
        char* end = current + numPages * PageCache::PAGE_SIZE;

        // a. 将第一个块作为链表的头
        void* head = current;
        void* tail = current;
        current += blockSize;

        // b. 遍历并链接所有剩余的块
        while (current + blockSize <= end) {
            *reinterpret_cast<void**>(tail) = current;
            tail = current;
            current += blockSize;
        }

        // c. 链表尾部指向 nullptr
        *reinterpret_cast<void**>(tail) = nullptr;

        // 5. [核心改进] 不再自己截留，而是返回整个新链表的头指针
        return head;
    }

    // 微调版：CentralCache::fetchRange
    void* CentralCache::fetchRange(size_t index, size_t batchNum)
    {
        assert(index < FREE_LIST_SIZE);

        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        void* result = nullptr;
        try {
            void* head = centralFreeList[index].load(std::memory_order_relaxed);

            // 如果自由链表为空，则先去 PageCache 补货
            if (!head) {
                // [微调之处]
                // 1. 调用新版的 fetchFromPageCache 获取一个完整的补货链表
                void* newBlocks = fetchFromPageCache(index);
                // 2. 将补货得到的链表放入我们的自由链表
                centralFreeList[index].store(newBlocks, std::memory_order_relaxed);
                // 3. 更新 head，让它指向新补的货
                head = newBlocks;
            }

            // 从这里开始，逻辑和之前一样，都是从 head 指向的链表中取货
            if (head) {
                result = head;

                void* current = head;
                size_t count = 1;
                while (count < batchNum && *reinterpret_cast<void**>(current) != nullptr) {
                    current = *reinterpret_cast<void**>(current);
                    count++;
                }

                void* nextBatch = *reinterpret_cast<void**>(current);
                *reinterpret_cast<void**>(current) = nullptr;

                centralFreeList[index].store(nextBatch, std::memory_order_relaxed);
            }
        } catch (...) {
            locks_[index].clear(std::memory_order_release);
            throw;
        }

        locks_[index].clear(std::memory_order_release);
        return result;
    }

    void CentralCache::returnRange(void *start, size_t size, size_t index) {
        assert(index < FREE_LIST_SIZE);

        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        try {
            void* tail = start;
            while (*reinterpret_cast<void**>(tail) != nullptr) {
                tail = *reinterpret_cast<void**>(tail);
            }
            void* oldList = centralFreeList[index].load(std::memory_order_relaxed);
            *reinterpret_cast<void**>(tail) = oldList;
            centralFreeList[index].store(start, std::memory_order_release);
        }
        catch (...) {
            locks_[index].clear(std::memory_order_release);
            throw;
        }

        locks_[index].clear(std::memory_order_release);
    }

}