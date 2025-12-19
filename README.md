# KV存储引擎

本项目是基于跳表实现的轻量级键值型存储引擎，使用C++实现。插入数据、删除数据、查询数据、数据展示、数据落盘、文件加载数据，以及数据库大小显示。

# 项目文件结构

```
Skiplist-CPP/
├── include/              # 头文件目录
│   ├── Node.h           # 跳表节点类定义
│   ├── SkipList.h       # 跳表核心实现（模板类）
│   └── KVStore.h        # KV存储引擎封装（支持持久化）
├── stress-test/         # 压力测试
│   └── stress_test.cpp  # 多线程并发性能测试
├── main.cpp             # 示例程序
├── CMakeLists.txt       # CMake 构建配置
├── store/               # 数据持久化文件存放目录
└── README.md            # 项目说明文档
```

# 核心接口

## KVStore 接口（推荐使用）

KVStore 是对 SkipList 的高层封装，提供了自动持久化功能：

* `put(key, value)` - 插入或更新键值对
* `get(key, value)` - 查询键对应的值
* `del(key)` - 删除指定键
* `clear()` - 清空所有数据
* `dump()` - 手动持久化数据到磁盘
* `load()` - 从磁盘加载数据（构造时自动调用）

## SkipList 接口（底层实现）

SkipList 是线程安全的跳表实现，支持泛型键值对：

* `insert_element(key, value)` - 插入元素（若存在则更新）
* `search_element(key, value)` - 查找元素
* `delete_element(key)` - 删除元素
* `process_all(func)` - 遍历所有元素（用于持久化等场景）
* `clear()` - 清空跳表


# 存储引擎数据表现（待更新）

## 插入操作

跳表树高：18 

采用随机插入数据测试：


|插入数据规模（万条） |耗时（秒） | 
|---|---|
|10 |0.316763 |
|50 |1.86778 |
|100 |4.10648 |


每秒可处理写请求数（QPS）: 24.39w

## 取数据操作

|取数据规模（万条） |耗时（秒） | 
|---|---|
|10|0.47148 |
|50|2.56373 |
|100|5.43204 |

每秒可处理读请求数（QPS）: 18.41w

# 项目运行方式

## 编译项目

使用 CMake 构建系统：

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目（Release 模式以获得最佳性能）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build .
```

## 运行示例程序

```bash
# 在 build 目录下
./bin/skiplist
```

## 运行压力测试

压力测试程序位于 `stress-test/` 目录，需要单独编译：

```bash
# 在项目根目录
g++ -std=c++20 -O2 -pthread -I./include stress-test/stress_test.cpp -o stress_test
./stress_test
```

## 在自己的项目中使用

本项目采用 header-only 设计，只需包含头文件即可使用：

```cpp
#include "KVStore.h"  // 或 #include "SkipList.h"

int main() {
    KVStore<int, std::string> kvStore("./data/store.txt");
    kvStore.put(1, "hello");
    
    std::string value;
    if (kvStore.get(1, value)) {
        std::cout << value << std::endl;
    }
    return 0;
}
```

# 核心特性

* ✅ **线程安全**：使用 `std::shared_mutex` 实现读写锁，查询操作支持并发读，写操作独占
* ✅ **内存安全**：完善的析构函数，避免内存泄漏；禁用拷贝构造防止 double free
* ✅ **自动持久化**：KVStore 在析构时自动保存数据，启动时自动加载
* ✅ **泛型支持**：基于模板实现，支持任意可比较的键类型和可序列化的值类型
* ✅ **现代 C++**：使用 C++20 标准，采用 `std::vector` 替代原生指针数组

# 待优化

* 增加 `size()` 接口返回当前元素数量
* 支持自定义比较函数，使 key 类型更灵活
* 压力测试脚本自动化（集成到 CMake 测试框架）
* 持久化格式优化（考虑二进制格式以提升性能）
* 增加范围查询接口（利用跳表有序性）
* 添加 raft 一致性协议，构建分布式存储系统
* 提供 HTTP 服务接口，对外提供分布式 KV 存储服务

可以尝试：
1.  **Generic Type Support**: 在 Load 时如何处理 `int` 类型的 Key？(需要特化模板或类型萃取)。
2.  **WAL**: 实现“写前日志”，防止程序崩溃导致内存数据丢失。
3.  **Bloom Filter**: 在 Search 之前先用布隆过滤器判断 Key 是否存在，减少不必要的跳表查询。
4.  **Memory Optimization**: `Node` 中使用 `std::vector` 虽然方便，但会导致双重内存分配（Node 一次，Vector 内容一次）。尝试使用 **Flexible Array Member** 或手动管理内存块，使 Node 内存布局更加紧凑 (Flat)。

# 参考与致谢

本项目基于 [Skiplist-CPP](https://github.com/youngyangyang04/Skiplist-CPP) 进行二次开发和优化。

[代码随想录网站](https://programmercarl.com)