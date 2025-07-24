#pragma once
#include <cstddef>
#include <algorithm>

namespace MemoryPool {
    // constexpt：constant expression，常量表达式，要求在编译时初始化

    // 对齐数
    constexpr size_t ALIGNMENT = 8;

    // 线程缓存能处理的最大对象，256KB，超过者不经过ThreadCache的优化路径
    constexpr size_t MAX_BYTES = 256*1024;

    // 自由链表数组的大小，从8B到256KB
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES/ALIGNMENT;

    // 处理计算大小有关的工具类
    class SizeClass {
        public:

        // 向上对齐
        // inline:建议编译器“直接把函数复制到调用处”，来减少函数调用的开销
        static inline size_t roundUp(size_t bytes) {
            // 向上取整：(bytes + 7) & ~7
            // 1. +7：让每个数字到达下一个8的倍数区间
            // 2. & ~7：把数字向下舍入到最接近的8的倍数，具体来说：
            // 2.1 ~7：对7按位取反，得到后三位全是0的数字
            // 2.2 对所求数字按位与~7，按位与的特点是“全1才1”，则所得结果后三位全部变为0
            // 2.3 这就是我们所要的“向上对齐”
            return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        // 计算自由链表索引，方法：（对齐大小 / 8） - 1（-1是因为索引从0开始）
        static inline size_t getIndex(size_t bytes) {
            return (roundUp(bytes)/ALIGNMENT)-1;
        }
    };
}