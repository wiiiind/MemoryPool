#pragma once

#include <cstddef> // 为了 size_t


// 一个Slot，本质上是一个链表节点
struct Slot {
    Slot* next;
};

class MemoryPool {
public:
    // MemoryPool实例负责管理一种特定大小的Slot内存。
    // 它从大的Block内存块中，切割出不同大小的Slot。

    MemoryPool(size_t BlockSize = 4096);    // 默认申请一个4096字节的Block

    ~MemoryPool();                          // 析构函数

    void init(size_t slotSize);             // 初始化不同大小的槽

    void* allocate();                       // 分配一个Slot

    void deallocate(void* p);               // 回收一个Slot

private:
    void allocateNewBlock();                // 当已有的Block装不下之后要申请新的
                                            // 这个函数是内部使用，不需要暴露

// 注意：这里有两个 private，是笔误，已合并
private:
    size_t m_BlockSize;   // 内存池的块大小
    size_t m_SlotSize;    // 每个Slot的大小

    Slot* m_firstBlock;   // 指向第一个内存块的指针
    Slot* m_curSlot;      // 指向下一个可以使用的Slot
    Slot* m_lastSlot;     // 指向内存块的末尾，用于判断是否越界
    Slot* m_freeList;     // 指向被回收的Slot链表的头节点
};

// --- 全局常量定义 ---
constexpr int MEMORY_POOL_NUM = 64;   // 内存池（哈希桶）的数量
constexpr int SLOT_BASE_SIZE = 8;     // Slot基础大小，即8字节
constexpr int MAX_SLOT_SIZE = 512;    // 内存池能处理的最大的Slot大小

class HashBucket {
public:
    static void initMemoryPool();                   // 初始化所有MemoryPool实例
    static void* useMemory(size_t size);             // 分配内存
    static void freeMemory(void* ptr, size_t size); // 回收内存

private:
    static MemoryPool& getMemoryPool(int index);    // 根据索引取得MemoryPool

    static MemoryPool m_pools[MEMORY_POOL_NUM];     // 静态数组，存储所有的MemoryPool实例
};

// --- 用户接口模板函数 ---
#include <new>      // 为了 placement new
#include <utility>  // 为了 std::forward

// 模板的定义必须在头文件(.h)中
template<typename T, typename... Args>
T* newElement(Args&&... args) {

    T* p = nullptr;
    // 先根据对象需要的内存大小分配内存，返回取得的内存地址p
    p = static_cast<T*>(HashBucket::useMemory(sizeof(T)));
    if (p) {
        // 2. 在已分配的内存上构造对象 (Placement New)
        // 在地址p上创建T类型的对象，并将参数完美转发给T的构造函数
        new(p) T(std::forward<Args>(args)...);
    }
    return p;
}

// deleteElement 模板函数，用于销毁对象
template<typename T>
void deleteElement(T* p) {
    if (!p) return;
    // 1. 调用析构函数
    p->~T();
    // 2. 归还内存
    HashBucket::freeMemory(p, sizeof(T));
}