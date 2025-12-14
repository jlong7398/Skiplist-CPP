# Skiplist-CPP 深度实战教程 (7天计划)

这份文档不仅是计划，更是一份**手把手的实现指南**。它将指导你从零开始，使用现代 C++ 构建一个工业级结构的 KV 存储引擎。

我们不再单纯复制原项目，而是**彻底重构**，解决原有的全局变量、线程不安全、职责不清等问题。

---

## 🛠️ 环境准备

你需要：
*   C++ 编译器 (GCC 4.8+ 或 Clang 3.3+, 支持 C++11)
*   CMake (3.10+)
*   Linux/macOS 环境 (推荐) 或 Windows (WSL)

---

## Day 1: 工程搭建与基础设施

**目标**：建立规范的 C++ 工程结构，完成构建脚本，并实现最基础的 `Node` 类。

### 1.1 目录结构
请在你的工作区创建以下目录树：
```text
Skiplist-KV/
├── CMakeLists.txt        # 构建脚本
├── include/
│   ├── Node.h            # 节点定义
│   └── SkipList.h        # 跳表核心
├── src/
│   └── main.cpp          # 测试入口
└── data/                 # 存放数据文件
```

### 1.2 编写 `CMakeLists.txt`
这是现代 C++ 项目的标配。

```cmake
cmake_minimum_required(VERSION 3.10)
project(Skiplist-KV)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 编译所有源文件
file(GLOB SOURCES "src/*.cpp")

add_executable(skiplist_kv ${SOURCES})
```

### 1.3 实现核心节点 `Node.h`
`Node` 是跳表的基本单元。

*   **设计决策**：使用 `template` 支持任意类型。
*   **代码实现**：

```cpp
// include/Node.h
#pragma once
#include <vector>

template <typename K, typename V>
class Node {
public:
    K key;
    V value;
    int node_level; // 该节点的层级

    // forward[i] 表示该节点在第 i 层的下一个节点
    // 使用 std::vector 替代原生的 Node** 数组，内存管理更安全
    // 或者为了追求极致性能保持 Node**，这里我们演示更 C++11 的写法
    std::vector<Node<K, V>*> forward;

    Node(K k, V v, int level) : key(k), value(v), node_level(level) {
        // 初始化 forward 数组，大小为 level + 1，全部置空
        this->forward.resize(level + 1, nullptr);
    }

    ~Node() {
        // vector 会自动释放内存，无需手动 delete []
    }
};
```

---

## Day 2: 跳表核心逻辑 (基础版)

**目标**：实现 `SkipList` 类的插入与查询功能。这是最核心的数据结构部分。

### 2.1 定义 `SkipList.h`
注意：模板类的声明和实现通常都要放在头文件中。

```cpp
// include/SkipList.h
#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <mutex>
#include "Node.h"

// 默认最大层数和概率
const int MAX_LEVEL = 32;
const double P_FACTOR = 0.25;

template <typename K, typename V>
class SkipList {
private:
    Node<K, V>* _header;
    int _current_level;
    int _element_count;
    std::mt19937 _rng;
    std::uniform_real_distribution<double> _dist;

private:
    // 获取随机层数
    int get_random_level() {
        int lvl = 0;
        while (_dist(_rng) < P_FACTOR && lvl < MAX_LEVEL) {
            lvl++;
        }
        return lvl;
    }

public:
    SkipList() : _current_level(0), _element_count(0) {
        // 头节点 key/value 不重要，层数设为最大
        K k; V v;
        _header = new Node<K, V>(k, v, MAX_LEVEL);

        // 初始化随机数生成器
        std::random_device rd;
        _rng = std::mt19937(rd());
        _dist = std::uniform_real_distribution<double>(0.0, 1.0);
    }

    ~SkipList() {
        Node<K, V>* current = _header;
        while (current) {
            Node<K, V>* next = current->forward[0];
            delete current;
            current = next;
        }
    }

    // 核心接口声明
    bool insert_element(K key, V value);
    bool search_element(K key, V& value); // 查找到的值存入 value 引用
};
```

### 2.2 实现 `search_element`
逻辑：从最高层出发，如果右边的 key 比目标小，就向右走；否则向下走。

```cpp
template <typename K, typename V>
bool SkipList<K, V>::search_element(K key, V& value) {
    Node<K, V>* current = _header;

    // 从最高层向下遍历
    for (int i = _current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }

    // 到达第0层，current 指向的是 < key 的最后一个节点
    // 所以检查 current->forward[0]
    current = current->forward[0];

    if (current && current->key == key) {
        value = current->value;
        return true;
    }
    return false;
}
```

### 2.3 实现 `insert_element`
逻辑：这是跳表最复杂的部分。你需要一个 `update` 数组记录每一层下降的位置（即新节点的前驱）。

```cpp
template <typename K, typename V>
bool SkipList<K, V>::insert_element(K key, V value) {
    Node<K, V>* current = _header;
    std::vector<Node<K, V>*> update(MAX_LEVEL + 1); // 用于记录每层的前驱节点

    // 1. 寻找插入位置
    for (int i = _current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // 2. 检查 key 是否已存在
    current = current->forward[0];
    if (current && current->key == key) {
        // 存在则更新值，或者返回 false 表示不允许重复
        return false;
    }

    // 3. 生成新节点层数
    int random_level = get_random_level();

    // 如果新层数超过当前最大层数，需要更新 update 数组
    if (random_level > _current_level) {
        for (int i = _current_level + 1; i <= random_level; i++) {
            update[i] = _header;
        }
        _current_level = random_level;
    }

    // 4. 创建并链接新节点
    Node<K, V>* new_node = new Node<K, V>(key, value, random_level);
    for (int i = 0; i <= random_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    _element_count++;
    return true;
}
```

---

## Day 3: 删除与并发安全

**目标**：完善 `delete` 功能，并引入线程锁。

### 3.1 实现 `delete_element`
在 `SkipList.h` 中添加：

```cpp
template <typename K, typename V>
bool SkipList<K, V>::delete_element(K key) {
    Node<K, V>* current = _header;
    std::vector<Node<K, V>*> update(MAX_LEVEL + 1);

    // 1. 寻找目标的前驱
    for (int i = _current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    // 2. 如果没找到
    if (!current || current->key != key) {
        return false;
    }

    // 3. 逐层断开链接
    for (int i = 0; i <= _current_level; i++) {
        // 如果这一层没有指向目标节点，说明上面层级已经没有目标节点了
        if (update[i]->forward[i] != current) break;
        update[i]->forward[i] = current->forward[i];
    }

    // 4. 调整当前层高（如果删除的是最高层的节点）
    while (_current_level > 0 && _header->forward[_current_level] == nullptr) {
        _current_level--;
    }

    delete current;
    _element_count--;
    return true;
}
```

### 3.2 引入并发控制 (RAII)
我们要让 SkipList 线程安全。
1.  在 `SkipList` 类中添加成员变量：`std::mutex _mtx;`。
2.  修改 **insert** 和 **delete** 方法，在函数开头加锁：

```cpp
bool insert_element(K key, V value) {
    std::lock_guard<std::mutex> lock(_mtx); // 自动加锁，作用域结束自动解锁
    // ... 原有逻辑 ...
}

bool delete_element(K key) {
    std::lock_guard<std::mutex> lock(_mtx);
    // ... 原有逻辑 ...
}
```
*注意：`search_element` 是否加锁取决于你是否允许“读到脏数据”。为了绝对安全，建议也加上 `lock_guard`。进阶可以改用 `std::shared_mutex` 实现读写分离。*

---

## Day 4: 封装 KVStore 引擎

**目标**：职责分离。`SkipList` 只是个数据结构，我们需要一个 `KVStore` 类来管理它和持久化逻辑。

### 4.1 定义 `KVStore.h`

```cpp
// include/KVStore.h
#pragma once
#include "SkipList.h"
#include <string>

template <typename K, typename V>
class KVStore {
private:
    SkipList<K, V> _skiplist;
    std::string _file_path;

public:
    KVStore(const std::string& path) : _file_path(path) {
        // 后续在这里加载数据
    }

    ~KVStore() {
        // 后续在这里保存数据
    }

    void put(K key, V value) {
        _skiplist.insert_element(key, value);
    }

    bool get(K key, V& value) {
        return _skiplist.search_element(key, value);
    }

    void del(K key) {
        _skiplist.delete_element(key);
    }
};
```
现在你的架构变成了：User -> KVStore -> SkipList。

---

## Day 5: 实现持久化 (Dump & Load)

**目标**：将内存数据保存到文件。

### 5.1 文件读写工具
为了保持 `SkipList` 的纯洁性，建议在 `KVStore` 中实现文件 I/O，或者让 `SkipList` 提供遍历接口。
为了简单起见，我们先给 `SkipList` 加一个 `dump_file` 方法（虽然这有点耦合，但作为学习过程可以接受），或者更好的是：**为 SkipList 实现迭代器**。

**简易方案：在 KVStore 中通过友元或公有接口遍历**。
假设我们在 `SkipList` 中加一个简单的遍历函数：
```cpp
// SkipList.h 中
template <typename Func>
void traverse(Func func) {
    Node<K, V>* node = _header->forward[0];
    while (node) {
        func(node->key, node->value);
        node = node->forward[0];
    }
}
```

### 5.2 实现 Dump (Save)
在 `KVStore.h` 中：
```cpp
#include <fstream>

void sync() {
    std::ofstream out(_file_path);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing: " << _file_path << std::endl;
        return;
    }

    // 使用 lambda 回调写入文件
    _skiplist.traverse([&](K key, V value) {
        out << key << ":" << value << "\n";
    });

    out.close();
}
```
*记得在 `KVStore` 析构函数中调用 `sync()`。*

### 5.3 实现 Load
在 `KVStore` 构造函数中：
```cpp
#include <sstream>

KVStore(const std::string& path) : _file_path(path) {
    std::ifstream in(_file_path);
    if (!in.is_open()) return; // 文件不存在可能是第一次运行

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string key_str, val_str;

        // 假设格式是 key:value，简单解析
        if (std::getline(ss, key_str, ':') && std::getline(ss, val_str)) {
            // 这里假设 K, V 都是 string，如果是 int 需要 stoi 转换
            // 为了通用性，Day 1 的模板最好实例化为 <std::string, std::string>
            _skiplist.insert_element(key_str, val_str);
        }
    }
    in.close();
}
```

---

## Day 6: 优化与测试

**目标**：编写测试代码，验证功能。

### 6.1 编写 `main.cpp`
```cpp
// src/main.cpp
#include <iostream>
#include "../include/KVStore.h"

int main() {
    KVStore<std::string, std::string> store("data/dump.txt");

    // 交互式测试
    while (true) {
        std::cout << "> ";
        std::string command;
        std::cin >> command;

        if (command == "SET") {
            std::string key, value;
            std::cin >> key >> value;
            store.put(key, value);
            std::cout << "OK" << std::endl;
        } else if (command == "GET") {
            std::string key;
            std::cin >> key;
            std::string value;
            if (store.get(key, value)) {
                std::cout << value << std::endl;
            } else {
                std::cout << "(nil)" << std::endl;
            }
        } else if (command == "DEL") {
            std::string key;
            std::cin >> key;
            store.del(key);
            std::cout << "OK" << std::endl;
        } else if (command == "EXIT") {
            break;
        }
    }
    return 0;
}
```

---

## Day 7: 性能测试与总结

**目标**：压力测试，评估性能，并总结学习成果。

1.  **压力测试**：编写一个脚本，循环插入 10万条 数据，计算耗时。
    ```cpp
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<100000; ++i) store.put(std::to_string(i), "test");
    auto end = std::chrono::high_resolution_clock::now();
    ```
2.  **瓶颈分析**：如果慢，慢在哪里？是锁竞争？还是文件 I/O？
    *   (提示：每次操作都写磁盘是最慢的。通常我们只在程序退出时 Dump，或者通过 WAL (Write Ahead Log) 优化。本教程只做了退出时 Dump。)
3.  **总结**：回顾所有代码，确保理解每一行。

---

## 💡 进阶思考 (Bonus)
完成上述一周任务后，你可以尝试：
1.  **Generic Type Support**: 在 Load 时如何处理 `int` 类型的 Key？(需要特化模板或类型萃取)。
2.  **WAL**: 实现“写前日志”，防止程序崩溃导致内存数据丢失。
3.  **Bloom Filter**: 在 Search 之前先用布隆过滤器判断 Key 是否存在，减少不必要的跳表查询。
