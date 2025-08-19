#pragma once
#include "Common.h"
#include <map>
#include <mutex>

namespace MemoryPool {
    class PageCache {
    public:
        static const size_t PAGE_SIZE = 4096;   // 4K

        // 单例模式
        static PageCache& getInstance() {
            static PageCache instance;
            return instance;
        }

        // 分配numPages页Span，返回值是指向内存的指针
        void* allocateSpan(size_t numPages);

        // 释放一个Span
        void deallocateSpan(void* ptr, size_t numPages);

    private:
        // 构造函数私有化
        PageCache() = default;
        // 类拷贝函数，=delete是C++11的新语法，会在试图调用时报错
        PageCache(const PageCache&) = delete;
        // 类拷贝赋值运算符
        PageCache& operator=(const PageCache&) = delete;

        // 私有辅助函数：没有适合的空闲内存时向系统申请
        void* systemAlloc(size_t numPages);

    private:
        // 一段连续的页，并不包含数据，只描述数据
        struct Span {
            void* pageAddr;     // 地址
            size_t numPages;    // 页数
            Span* next;         // 用于连成链表
        };

        // 空闲的Span链表，key是页的数量
        // 用map可以按页数自动排序，方便我们找到>=所需页数的空闲块
        std::map<size_t, Span*> freeSpans_;

        // 页地址和Span的映射
        std::map<void*, Span*> spanMap_;

        // 全局锁：PageCache对所有线程共享
        std::mutex mutex_;
    };
}