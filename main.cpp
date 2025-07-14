#include <iostream>
#include "MemoryPool.h"

// 定义一个简单的测试类，大小为 12 字节 (3 * sizeof(int))
// 这将使内存池选择 16 字节的内存槽
class SimpleObject {
public:
    SimpleObject(int v1, int v2, int v3) : val1(v1), val2(v2), val3(v3) {
        std::cout << "[Constructor] SimpleObject(" << val1 << ", " << val2 << ", " << v3 << ") is created at address: " << this << std::endl;
    }
    ~SimpleObject() {
        std::cout << "[Destructor] SimpleObject at address " << this << " is being destroyed." << std::endl;
    }

private:
    int val1, val2, val3;
};

int main() {
    // -----------------------------------------------------------------
    // 步骤 1: 初始化内存池
    // -----------------------------------------------------------------
    std::cout << "--- STEP 1: Initializing Memory Pools ---" << std::endl;
    HashBucket::initMemoryPool();
    std::cout << "Initialization complete.\n" << std::endl;

    // -----------------------------------------------------------------
    // 步骤 2: 第一次分配
    // 这次分配会从一个全新的内存块(Block)中获取内存
    // -----------------------------------------------------------------
    std::cout << "--- STEP 2: First Allocation ---" << std::endl;
    std::cout << "Requesting memory for SimpleObject (size = " << sizeof(SimpleObject) << " bytes)..." << std::endl;
    SimpleObject* obj1 = newElement<SimpleObject>(1, 2, 3);
    if (!obj1) {
        std::cerr << "Allocation failed!" << std::endl;
        return 1;
    }
    std::cout << "Allocation successful.\n" << std::endl;

    // -----------------------------------------------------------------
    // 步骤 3: 第一次释放
    // 这次释放会把内存归还给内存池的 freeList，而不是操作系统
    // -----------------------------------------------------------------
    std::cout << "--- STEP 3: First Deallocation ---" << std::endl;
    std::cout << "Deallocating obj1 at address: " << obj1 << "..." << std::endl;
    deleteElement(obj1);
    std::cout << "Deallocation successful. The memory slot is now in the pool's freeList.\n" << std::endl;

    // -----------------------------------------------------------------
    // 步骤 4: 第二次分配 (关键步骤)
    // 这次分配将复用刚才被释放的内存槽，速度会非常快
    // -----------------------------------------------------------------
    std::cout << "--- STEP 4: Second Allocation (Testing Reuse) ---" << std::endl;
    std::cout << "Requesting another SimpleObject..." << std::endl;
    SimpleObject* obj2 = newElement<SimpleObject>(4, 5, 6);
    if (!obj2) {
        std::cerr << "Second allocation failed!" << std::endl;
        return 1;
    }
    std::cout << "Second allocation successful.\n" << std::endl;

    // -----------------------------------------------------------------
    // 步骤 5: 验证内存复用
    // -----------------------------------------------------------------
    std::cout << "--- STEP 5: Verifying Memory Reuse ---" << std::endl;
    std::cout << "Address of first object (obj1): " << obj1 << std::endl;
    std::cout << "Address of second object (obj2): " << obj2 << std::endl;
    if (obj1 == obj2) {
        std::cout << "SUCCESS: The memory address is the same. Memory was reused from the freeList!" << std::endl;
    } else {
        std::cout << "FAILURE: Memory was not reused." << std::endl;
    }
    std::cout << std::endl;


    // -----------------------------------------------------------------
    // 步骤 6: 最终清理
    // -----------------------------------------------------------------
    std::cout << "--- STEP 6: Final Cleanup ---" << std::endl;
    deleteElement(obj2);
    std::cout << "Cleanup complete. Test finished." << std::endl;

    return 0;
}