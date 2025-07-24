#include  "../include/PageCache.h"
#include <sys/mman.h>   // mmap，这是Linux向OS申请内存的方法
#include <cassert>

namespace MemoryPool {
    void *PageCache::systemAlloc(size_t numPages) {
        size_t size = numPages * PAGE_SIZE;

        void* ptr = mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS,-1,0);

        if (ptr == MAP_FAILED)
            return nullptr;

        return ptr;
    }
    void *PageCache::allocateSpan(size_t numPages) {
        assert(numPages > 0);

        std::lock_guard lock(mutex_);

        const auto it = freeSpans_.lower_bound(numPages);
        if (it!=freeSpans_.end()) {
            Span* span = it->second;

            if (span->next) {
                it->second = span->next;
            }
            else {
                freeSpans_.erase(it);
            }

            if (span->numPages>numPages) {
                Span* newSpan = new Span();
                newSpan->pageAddr = static_cast<char*>(span->pageAddr) + span->numPages * PAGE_SIZE;
                newSpan->numPages = span->numPages - numPages;
                newSpan->next = nullptr;

                auto& list = freeSpans_[newSpan->numPages];
                newSpan->next = list;
                list = newSpan;

                span->numPages = numPages;
            }

            spanMap_[span->pageAddr] = span;
            return span->pageAddr;
        }

        void* memory  = systemAlloc(numPages);
        if (!memory) return nullptr;

        Span* newSpan = new Span();
        newSpan->pageAddr = memory;
        newSpan->numPages = numPages;
        newSpan->next = nullptr;

        spanMap_[memory] = newSpan;
        return memory;
    }

    void PageCache::deallocateSpan(void* ptr, size_t numPages) {
        std::lock_guard lock(mutex_);

        auto it = spanMap_.find(ptr);
        assert(it != spanMap_.end());

        Span* span = it->second;
        assert(span->numPages == numPages);

        void* nextAddr = static_cast<char*>(ptr) + numPages * PAGE_SIZE;
        auto nextIt = spanMap_.find(nextAddr);

        if (nextIt != spanMap_.end()) {
            Span* nextSpan = nextIt->second;

            auto freeIt = freeSpans_.find(nextSpan->numPages);
            if (freeIt != freeSpans_.end()) {
                Span* current = freeIt->second;
                Span* prev = nullptr;
                while (current && current != nextSpan) {
                    prev = current;
                    current = current->next;
                }
                if (current) {
                    if (prev) prev->next = current->next;
                    else freeIt->second = current->next;
                    if (!freeIt->second) freeSpans_.erase(freeIt);

                    span->numPages+=nextSpan->numPages;
                    spanMap_.erase(nextAddr);
                    delete nextSpan;
                }
            }
        }
        auto& list = freeSpans_[span->numPages];
        span->next = list;
        list = span;
    }

}