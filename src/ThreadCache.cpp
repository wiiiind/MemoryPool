#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"
#include <cassert>

namespace MemoryPool
{

void* ThreadCache::allocate(size_t size)
{
    if (size > MAX_BYTES) {
        return malloc(size);
    }
    
    size_t index = SizeClass::getIndex(size);
    
    if (freeList_[index]) {
        void* ptr = freeList_[index];
        freeList_[index] = *reinterpret_cast<void**>(ptr);
        freeListSize_[index]--;
        return ptr;
    } else {
        return fetchFromCentralCache(index);
    }
}

void ThreadCache::deallocate(void* ptr, size_t size)
{
    if (size > MAX_BYTES) {
        free(ptr);
        return;
    }

    size_t index = SizeClass::getIndex(size);

    *reinterpret_cast<void**>(ptr) = freeList_[index];
    freeList_[index] = ptr;
    freeListSize_[index]++;

    const size_t HIGH_WATER_MARK = 20; 
    if (freeListSize_[index] > HIGH_WATER_MARK) {
        returnToCentralCache(index);
    }
}

void* ThreadCache::fetchFromCentralCache(size_t index)
{
    void* ptr = CentralCache::getInstance().fetchRange(index);
    return ptr;
}

void ThreadCache::returnToCentralCache(size_t index)
{
    void* start = freeList_[index];
    void* end = start;
    size_t returnCount = freeListSize_[index] / 2;
    if (returnCount == 0) return;

    for(size_t i = 0; i < returnCount - 1; ++i) {
        end = *reinterpret_cast<void**>(end);
    }

    freeList_[index] = *reinterpret_cast<void**>(end);
    *reinterpret_cast<void**>(end) = nullptr;

    freeListSize_[index] -= returnCount;

    size_t blockSize = (index + 1) * ALIGNMENT;
    CentralCache::getInstance().returnRange(start, returnCount * blockSize, index);
}

} // namespace MemoryPool