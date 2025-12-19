// include/SkipList.h
#pragma once
#include <cmath>
#include <iostream>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <vector>

#include "Node.h"

#define STORE_FILE "store/dumpFile"

// 默认最大层数和概率
const int MAX_LEVEL = 32;
const double P_FACTOR = 0.5;

template <typename K, typename V>
class SkipList {
 private:
  Node<K, V>* header_;  // 头节点
  int current_level_;   // 当前层数
  int element_count_;   // skiplist当前元素个数

  std::shared_mutex mutex_;  // 互斥锁，用于线程安全

 private:
  int get_random_level() {
    // 使用局部静态变量替代类静态成员，避免类外定义的麻烦
    static thread_local std::mt19937 generator{std::random_device{}()};
    static thread_local std::uniform_real_distribution<double> distribution(
        0.0, 1.0);

    int lvl = 0;
    while (distribution(generator) < P_FACTOR && lvl < MAX_LEVEL) {
      lvl++;
    }
    return lvl;
  }

 public:
  SkipList() : current_level_(0), element_count_(0) {
    // 初始化头节点，层数为0，key和value为空
    K k;
    V v;
    header_ = new Node<K, V>(k, v, MAX_LEVEL);
  }

  // 禁止拷贝，防止Double Free
  SkipList(const SkipList& other) = delete;
  SkipList& operator=(const SkipList& other) = delete;

  // 析构函数，删除所有节点
  ~SkipList() {
    clear();
    delete header_;
  }

  // 核心接口声明
  bool insert_element(const K& key, const V& value);  // 插入新节点
  bool search_element(const K& key, V& value);        // 查找节点
  bool delete_element(const K& key);                  // 删除节点
  // 遍历接口
  template <typename Func>
  void process_all(Func func);
  // 清空跳表
  void clear();
};

// 清空跳表
template <typename K, typename V>
void SkipList<K, V>::clear() {
  Node<K, V>* current = header_->forward_[0];
  while (current) {
    Node<K, V>* next = current->forward_[0];
    delete current;
    current = next;
  }

  // 清空头节点的 forward 指针数组
  std::fill(header_->forward_.begin(), header_->forward_.end(), nullptr);
  // 重置状态
  current_level_ = 0;
  element_count_ = 0;
}

// 逻辑：从最高层出发，若右边的key比目标小，就向右走；否则向下走
template <typename K, typename V>
bool SkipList<K, V>::search_element(const K& key, V& value) {
  Node<K, V>* current = header_;

  std::shared_lock<std::shared_mutex> lock(mutex_);

  // 从最高层向下遍历，直到最后一层
  // 循环结束时，要么右边没有节点了，要么右边的key大于等于目标key
  for (int i = current_level_; i >= 0; --i) {
    // 向右遍历，直到找到大于等于 key 的节点
    while (current->forward_[i] && current->forward_[i]->key_ < key) {
      current = current->forward_[i];
    }
  }

  // 到达第0层，current指向的是 < key 的最后一个节点
  // 再向右遍历，找到 >= key 的节点
  current = current->forward_[0];
  if (current && current->key_ == key) {
    value = current->value_;
    return true;
  }
  return false;
}

// 供外部遍历所有节点 (e.g. KVStore dump)
template <typename K, typename V>
template <typename Func>
void SkipList<K, V>::process_all(Func func) {
  Node<K, V>* node = header_->forward_[0];
  while (node != nullptr) {
    func(node->key_, node->value_);
    node = node->forward_[0];
  }
}

// 需要一个update数组，用于记录每一层下降的位置（也就是新节点的前驱）
template <typename K, typename V>
bool SkipList<K, V>::insert_element(const K& key, const V& value) {
  Node<K, V>* current = header_;
  std::unique_lock<std::shared_mutex> lock(mutex_);
  // 使用圆括号初始化，创建大小为 MAX_LEVEL + 1 的 vector，所有元素初始化为
  // nullptr
  std::vector<Node<K, V>*> update(MAX_LEVEL + 1, nullptr);

  // 1. 寻找插入位置
  for (int i = current_level_; i >= 0; --i) {
    while (current->forward_[i] && current->forward_[i]->key_ < key) {
      current = current->forward_[i];
    }
    update[i] = current;
  }
  // 2. 检查 key 是否已存在
  current = current->forward_[0];
  if (current && current->key_ == key) {
    // 存在则更新值
    current->value_ = value;
    return true;
  }

  // 3. 生成新节点层数
  int random_level = get_random_level();

  // 如果新层数超过当前最大层数，需要更新 update 数组
  if (random_level > current_level_) {
    for (int i = current_level_ + 1; i <= random_level; i++) {
      // 更高层的前驱都设为头结点（在这些层上，头结点是唯一的前驱）
      update[i] = header_;
    }
    current_level_ = random_level;
  }

  // 4. 创建并链接新节点
  Node<K, V>* new_node = new Node<K, V>(key, value, random_level);
  for (int i = 0; i <= random_level; i++) {
    new_node->forward_[i] = update[i]->forward_[i];
    update[i]->forward_[i] = new_node;
  }

  element_count_++;
  return true;
}

template <typename K, typename V>

bool SkipList<K, V>::delete_element(const K& key) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  Node<K, V>* current = header_;
  // update 数组用于存储在每一层遍历过程中，待删除节点的前驱节点。
  // 这样在删除节点时，可以方便地重新连接跳表。
  std::vector<Node<K, V>*> update(MAX_LEVEL + 1);

  // 1. 查找待删除节点的前驱节点
  // 从跳表最高层开始向下查找，记录每一层中，key 的前驱节点到 update 数组。
  for (int i = current_level_; i >= 0; --i) {
    while (current->forward_[i] && current->forward_[i]->key_ < key) {
      current = current->forward_[i];
    }
    update[i] = current;  // update[i] 记录了在第 i 层，key 的前一个节点
  }

  // 经过上述循环，current 此时指向第 0 层上 key 的前驱节点。
  // 将 current 移动到第 0 层上，
  // 可能是目标节点，也可能是比目标节点大的第一个节点。
  current = current->forward_[0];

  // 2. 检查节点是否存在
  // 如果 current 为空 (表示 key 比所有节点都大) 或者 current 的 key 不匹配，
  // 则说明目标节点不存在，删除失败。
  if (current == nullptr || current->key_ != key) {
    return false;  // 目标节点不存在，删除失败
  }

  // 3. 从跳表中移除节点
  // 遍历所有层，如果 update[i] 指向的下一个节点是待删除节点 current，
  // 则更新 update[i] 的 forward 指针，使其跳过 current。
  // 之所以从第0层开始，是为了确保所有层级的链接都被正确调整。
  for (int i = 0; i <= current_level_; ++i) {
    // 如果这一层 update[i] 的 forward_ 指针不再指向 current，
    // 说明 current 节点在这一层及更高层并不存在，因此无需继续更新更高层。
    if (update[i]->forward_[i] != current) {
      break;
    }
    // 重新连接跳表节点，跳过待删除节点 current
    update[i]->forward_[i] = current->forward_[i];
  }

  // 4. 调整跳表的当前最高层级
  // 如果删除的节点是当前最高层上唯一的节点，导致该层变空，
  // 则需要降低 current_level_，以避免不必要的最高层遍历。
  while (current_level_ > 0 && header_->forward_[current_level_] == nullptr) {
    --current_level_;
  }

  // 5. 释放内存并更新计数
  delete current;
  element_count_--;
  return true;
}