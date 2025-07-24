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

    void* CentralCache::fetchFromPageCache(size_t index) {
        size_t blockSize = (index + 1) * ALIGNMENT;

        const size_t SPAN_PAGES = 8;
        size_t numPages = (blockSize < SPAN_PAGES * PageCache::PAGE_SIZE)
                          ? SPAN_PAGES
                          : (blockSize + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;

        void* spanStart = PageCache::getInstance().allocateSpan(numPages);
        if (!spanStart) {
            return nullptr;
        }

        char* current = static_cast<char*>(spanStart);
        char* end = current + numPages * PageCache::PAGE_SIZE;
        
        void* listHead = current;
        void* tail = current;
        current += blockSize;

        while (current + blockSize <= end) {
            *reinterpret_cast<void**>(tail) = current;
            tail = current;
            current += blockSize;
        }
        if (tail) {
            *reinterpret_cast<void**>(tail) = nullptr;
        }
        
        return listHead;
    }

    void* CentralCache::fetchRange(size_t index) {
        assert(index < FREE_LIST_SIZE);

        while (locks_[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        void* head = centralFreeList[index].load(std::memory_order_relaxed);
        if (head) {
            void* result = head;
            void* next = *reinterpret_cast<void**>(head);
            centralFreeList[index].store(next, std::memory_order_release);
            locks_[index].clear(std::memory_order_release);
            return result;
        }
        
        locks_[index].clear(std::memory_order_release);

        void* newBlocks = fetchFromPageCache(index);
        if (!newBlocks) {
            return nullptr;
        }

        void* result = newBlocks;
        void* rest = *reinterpret_cast<void**>(newBlocks);

        if (rest) {
            returnRange(rest, 0, index);
        }

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