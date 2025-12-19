cmake_minimum_required(VERSION 3.10)

# 声明工程名称
project(Skiplist VERSION 1.0 LANGUAGES CXX)

# 指定 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建类型，如果没有指定，默认使用 Debug 模式以便调试
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 定义源文件和头文件变量，将头文件加入目标可方便在 IDE 中查看
set(SOURCES main.cpp)
set(HEADERS 
    include/KVStore.h
    include/SkipList.h
    include/Node.h
)

# 添加头文件搜索路径
include_directories(include)

# 添加可执行文件
add_executable(skiplist ${SOURCES} ${HEADERS})

# 设置编译选项：开启常见警告
# 优化级别应由 CMAKE_BUILD_TYPE 控制 (Release 模式会自动加 -O2/-O3)
if(MSVC)
    target_compile_options(skiplist PRIVATE /W3 /permissive-)
else()
    target_compile_options(skiplist PRIVATE -Wall -Wextra)
endif()

# 查找并链接 Threads 库 (这是跨平台处理 pthread 的标准方式)
find_package(Threads REQUIRED)
target_link_libraries(skiplist PRIVATE Threads::Threads)

# 设置可执行文件的输出目录
# 使用 CMAKE_RUNTIME_OUTPUT_DIRECTORY 是更现代的做法
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Executable output path: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")