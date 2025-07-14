#include "../include/MemoryPool.h"
#include <cassert>

// 构造函+数
MemoryPool::MemoryPool(size_t BlockSize) {
    m_BlockSize = BlockSize;
    m_SlotSize = 0;
    m_firstBlock = nullptr;
    m_curSlot = nullptr;
    m_lastSlot = nullptr;
    m_freeList = nullptr;
}

// 初始化函数
void MemoryPool::init(size_t slotSize) {
    assert(slotSize > 0);		// 断言函数，确保slotSize大于0
    m_SlotSize = slotSize;      // 设置这个实例的Slot大小
}

MemoryPool::~MemoryPool(){
    // 其实就是把Block逐个delete掉
    Slot * currentBlock = m_firstBlock;
    while (currentBlock) {
        Slot * next = currentBlock ->next;
        // reinterpret_cast是强制类型转化，把Slot*转化为void*
        // void*只存储地址，不关注变量类型
        // delete要求传入void*类型的指针，因此需要转换
        // operator delete是C++的内存释放函数，用于释放通过new分配的内存
        // delete是一个运算符，而operator delete是一个函数，只释放内存，不调用析构函数
        // 我们申请的是一块内存而不是一个对象，所以不能使用delete
        operator delete(reinterpret_cast<void*>(currentBlock));
        currentBlock = next;
    }
}

// 申请一个新的大内存块
void MemoryPool::allocateNewBlock() {
    // 申请一块大内存:
    // 1. operator new 申请一块新的内存，大小是 m_BlockSize
    // 2. 把他转化为char*（通过reinterpret_cast<char*>），用于对内存进行字节级别操作
    char* newBlock = reinterpret_cast<char*>(operator new(m_BlockSize));

    // 把新申请的Block放到最前面
    // 注意，m_firstBlock是“指向头指针的指针”，用它指向我们新创建的节点
    // 所以更新头指针的时候，直接让next指向m_firstBlock，就已经是真正的第一个Block的位置了
    // m_firstBlock被强制转化为Slot*，即占用了一个Slot*大小的位置存储指针，m_curSlot定位时注意跳过
    reinterpret_cast<Slot*>(newBlock)->next = m_firstBlock;
    m_firstBlock = reinterpret_cast<Slot*>(newBlock);

    // 更新新Block的边界，注意Block最前面有一个Slot*大小的位置是指针
    m_curSlot = reinterpret_cast<Slot*>(newBlock + sizeof(Slot*));
    // m_lastSlot指向最后一个的Slot+1的位置，这样下一次allocate Slot的时候m_curSlot必定在m_lastSlot后面
    m_lastSlot = (Slot*)(newBlock + m_BlockSize - m_SlotSize);
}

// 分配一个Slot
void* MemoryPool::allocate() {

    // 1.先看看有没有回收的内存可用
    if (m_freeList) {
        Slot* result = m_freeList;
        m_freeList = m_freeList->next;
        return result;
    }

    // 2. 没有freeList用，直接从Block分配Slot

    if (m_curSlot > m_lastSlot || !m_firstBlock) {
        allocateNewBlock();
    }

    Slot* result = m_curSlot;
    // 计算地址，先吧Slot*转化为char*，然后移动，再转化回Slot*
    m_curSlot = reinterpret_cast<Slot*>(reinterpret_cast<char*>(m_curSlot) + m_SlotSize);
    return result;
}

// 回收一个Slot
void MemoryPool::deallocate(void* p) {
    if (!p) return;


    // 任何地址转为void*都是安全转化，安全转化只需要使用static_cast
    Slot* head = static_cast<Slot*>(p);
    head->next = m_freeList;
    m_freeList = head;
}

// 下面是HashBucket（哈基桶）

// 静态成员类型必须在类外定义
MemoryPool HashBucket::m_pools[MEMORY_POOL_NUM];

// 初始化
void HashBucket::initMemoryPool() {
    for (int i = 0; i < MEMORY_POOL_NUM; i++) {
        m_pools[i].init((i+1) * SLOT_BASE_SIZE);
    }
}

MemoryPool& HashBucket::getMemoryPool(int index) {
    return m_pools[index];
}

void *HashBucket::useMemory(size_t size) {
    if (size == 0) return nullptr;

    // 对于大内存需求，直接调用系统的
    if (size>MAX_SLOT_SIZE) return operator new(size);

    // 巧妙的向上取整算法：((size + N - 1) / N)
    // 这里不用向下取整是因为对于整除的数还要额外判断-1，不如向上取整，这样必定多了1，减到即可
    int index = ((size + SLOT_BASE_SIZE -1))/SLOT_BASE_SIZE-1;
    // 先调用HashBucket的getMemoryPool获取对应大小的内存池，
    // 然后让MemoryPool allocate Slot并返回地址
    return getMemoryPool(index).allocate();
}

void HashBucket::freeMemory(void *ptr, size_t size) {
    if (!ptr) return;

    if (size > MAX_SLOT_SIZE) {
        operator delete(ptr);
        return;
    }

    int index = ((size + SLOT_BASE_SIZE -1))/SLOT_BASE_SIZE-1;
    getMemoryPool(index).deallocate(ptr);
}

