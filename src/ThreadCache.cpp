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

    // 重构 fetchFromCentralCache
    void* ThreadCache::fetchFromCentralCache(size_t index)
{
    size_t blockSize = (index + 1) * ALIGNMENT;
    size_t batchNum = getBatchNum(blockSize);

    void* start = CentralCache::getInstance().fetchRange(index, batchNum);
    if (!start) {
        return nullptr;
    }

    void* result = start;
    void* next = *reinterpret_cast<void**>(start);

    if (next != nullptr) {
        freeList_[index] = next;
        size_t count = 0;
        void* current = next;
        while (current != nullptr) {
            current = *reinterpret_cast<void**>(current);
            count++;
        }
        freeListSize_[index] = count;
    } else {
        freeList_[index] = nullptr;
        freeListSize_[index] = 0;
    }

    return result;
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

    // 新增：计算批量获取内存块的数量
    size_t ThreadCache::getBatchNum(size_t size)
{
    // 定义：这是一个启发式算法，目标是让一次批量获取的总字节数趋向于一个合理值，
    // 但同时要避免小对象一次拿太多，大对象一次拿太少。

    // 基准：我们不希望一次性从 CentralCache 拿走超过 4KB 的内存。
    // 这是一个权衡：拿太少，与 CentralCache 交互太频繁；拿太多，可能造成单个 ThreadCache 囤积过多内存。
    constexpr size_t MAX_BATCH_BYTES = 4 * 1024; // 4KB

    // 1. 根据对象大小，设定一个期望的基础批量数 (baseNum)。
    // 思想：对象越小，一次可以拿越多；对象越大，一次拿得越少。
    size_t baseNum;
    if (size <= 32)       baseNum = 64;   // 64 * 32 = 2KB
    else if (size <= 64)  baseNum = 32;   // 32 * 64 = 2KB
    else if (size <= 128) baseNum = 16;   // 16 * 128 = 2KB
    else if (size <= 256) baseNum = 8;    // 8 * 256 = 2KB
    else if (size <= 512) baseNum = 4;    // 4 * 512 = 2KB
    else if (size <= 1024)baseNum = 2;    // 2 * 1024 = 2KB
    else                  baseNum = 1;    // 大于1KB的对象，每次只拿1个，避免浪费

    // 2. 根据 4KB 的上限，计算理论上最多能拿多少个 (maxNum)。
    // 确保 size 不为 0，避免除零错误。
    size_t maxNum = (size > 0) ? (MAX_BATCH_BYTES / size) : 1;
    if (maxNum == 0) maxNum = 1; // 至少要拿一个

    // 3. 最终的批量数，是基础批量数和最大批量数中的较小者。
    // 这确保了我们既不会拿得太少（遵循baseNum），也不会拿得太多（不超过maxNum的上限）。
    return std::min(baseNum, maxNum);
}
} // namespace MemoryPool