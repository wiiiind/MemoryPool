#include <iostream>
#include <thread>
#include <vector>
#include <chrono> // 使用更精确的计时器
#include <atomic> // 用于多线程下的安全计数

#include "MemoryPool.h"

// --- 我们把你写的测试代码复制到这里 ---

// 测试用例
class P1 
{
    int id_;
};

class P2 
{
    int id_[5];
};

class P3
{
    int id_[10];
};

class P4
{
    int id_[20];
};

// 使用 atomic 来安全地累加时间，避免多线程数据竞争
std::atomic<size_t> total_costtime_ms(0);

// 单轮次申请释放次数 线程数 轮次
void BenchmarkMemoryPool(size_t ntimes, size_t nworks, size_t rounds)
{

	std::vector<std::thread> vthread(nworks);
	total_costtime_ms = 0; // 重置计时器

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([=]() { // 使用值捕获 [=] 避免引用悬空
			std::vector<void*> p_vec; // 用来存储指针，防止编译器过度优化
            p_vec.reserve(ntimes * 4);

			for (size_t j = 0; j < rounds; ++j)
			{
				auto begin = std::chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++)
				{
                    p_vec.push_back(newElement<P1>());
                    p_vec.push_back(newElement<P2>());
                    p_vec.push_back(newElement<P3>());
                    p_vec.push_back(newElement<P4>());
				}
                // 真正要测的是分配和释放的完整流程
                for (size_t i = 0; i < ntimes; i++) {
                    deleteElement(static_cast<P1*>(p_vec[i*4]));
                    deleteElement(static_cast<P2*>(p_vec[i*4+1]));
                    deleteElement(static_cast<P3*>(p_vec[i*4+2]));
                    deleteElement(static_cast<P4*>(p_vec[i*4+3]));
                }
                p_vec.clear();

				auto end = std::chrono::high_resolution_clock::now();
				total_costtime_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%zu个线程并发执行%zu轮次，每轮次new/delete %zu*4次，总计花费：%zu ms (MemoryPool)\n", nworks, rounds, ntimes, total_costtime_ms.load());
}

void BenchmarkNew(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	total_costtime_ms = 0; // 重置计时器

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([=]() {
			std::vector<void*> p_vec;
            p_vec.reserve(ntimes * 4);
			for (size_t j = 0; j < rounds; ++j)
			{
				auto begin = std::chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++)
				{
                    p_vec.push_back(new P1);
                    p_vec.push_back(new P2);
                    p_vec.push_back(new P3);
                    p_vec.push_back(new P4);
				}
                for (size_t i = 0; i < ntimes; i++) {
                    delete static_cast<P1*>(p_vec[i*4]);
                    delete static_cast<P2*>(p_vec[i*4+1]);
                    delete static_cast<P3*>(p_vec[i*4+2]);
                    delete static_cast<P4*>(p_vec[i*4+3]);
                }
                p_vec.clear();
				auto end = std::chrono::high_resolution_clock::now();
				total_costtime_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%zu个线程并发执行%zu轮次，每轮次new/delete %zu*4次，总计花费：%zu ms (new/delete)\n", nworks, rounds, ntimes, total_costtime_ms.load());
}

// --- 这里是程序的入口 ---
int main()
{
	// system("chcp 65001");
    size_t ntimes = 10000;
    size_t nworks = 1;
    size_t rounds = 10;

    std::cout << "Benchmark starting..." << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;

    // 使用内存池接口前一定要先调用该函数
    HashBucket::initMemoryPool();
	BenchmarkMemoryPool(ntimes, nworks, rounds);

	std::cout << "--------------------------------------------------------" << std::endl;

	BenchmarkNew(ntimes, nworks, rounds);
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << "Benchmark finished." << std::endl;
	
	return 0;
}