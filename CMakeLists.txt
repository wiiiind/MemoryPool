# --- CMakeLists.txt for High-Performance Memory Pool ---

# 1. 设置CMake最低版本和项目名称
cmake_minimum_required(VERSION 3.10)
project(HighPerfMemoryPool)

# 2. 设置C++标准
# 我们的代码使用了C++17的特性（如thread_local的现代化用法），所以必须指定。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. 添加编译选项
# -Wall: 开启所有编译器警告，帮助我们写出更严谨的代码。
# -g:    生成调试信息，方便使用GDB等工具进行调试。
# -O2:   开启二级优化，让编译器优化代码以获得更好的性能。
# -pthread: 明确告知编译器和链接器我们要使用POSIX线程库。
add_compile_options(-Wall -g -O2 -pthread)

# 4. 查找多线程库
# find_package(Threads REQUIRED) 是现代CMake推荐的方式，
# 它会找到pthread库并创建一个名为 Threads::Threads 的导入目标。
find_package(Threads REQUIRED)

# 5. 定义目录变量
# 使用变量来表示目录，让脚本更清晰，更易于维护。
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# 6. 添加头文件目录
# 这样在.cpp文件中我们就可以直接 #include "Common.h" 而不是 #include "../include/Common.h"
include_directories(${INC_DIR})

# 7. 自动查找所有公共源文件
# 使用 file(GLOB ...) 可以自动将 src/ 目录下的所有 .cpp 文件加入列表，
# 当我们未来在 src/ 中添加新文件时，无需修改此CMakeLists.txt。
file(GLOB COMMON_SOURCES "${SRC_DIR}/*.cpp")

# 8. 创建可执行目标 (目标分离思想)

# 目标一：单元测试程序
# 用于验证功能的正确性。
add_executable(unit_test
        ${COMMON_SOURCES}           # 包含所有内存池的实现代码
        ${TEST_DIR}/UnitTest.cpp    # 包含单元测试的main函数
)

# 目标二：性能测试程序
# 用于衡量内存池的性能。
add_executable(perf_test
        ${COMMON_SOURCES}           # 包含所有内存池的实现代码
        ${TEST_DIR}/PerformanceTest.cpp # 包含性能测试的main函数
)

# 9. 链接库
# 为我们创建的两个目标链接它们所需要的库。
# 在这里，它们都需要多线程库。
target_link_libraries(unit_test PRIVATE Threads::Threads)
target_link_libraries(perf_test PRIVATE Threads::Threads)

# 10. (可选但推荐) 添加自定义命令，方便运行
# 这样我们就可以在命令行中通过 `make run_tests` 来直接运行程序。
add_custom_target(run_unit_test
        COMMAND ./unit_test         # 要执行的命令
        DEPENDS unit_test           # 确保在执行前，unit_test已经编译好了
        COMMENT "Running Unit Tests..." # 执行时显示的提示信息
)

add_custom_target(run_perf_test
        COMMAND ./perf_test
        DEPENDS perf_test
        COMMENT "Running Performance Benchmarks..."
)

# 打印一些有用的信息，方便确认配置
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source files: ${COMMON_SOURCES}")