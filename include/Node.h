// include/Node.h
#pragma once
#include <vector>

template <typename K, typename V>
class Node {
 public:
  K key_;
  V value_;
  int node_level_;  // 该节点的层级

  // forward[i] 表示该节点在第 i 层的下一个节点
  // 使用 std::vector 替代原生的 Node** 数组，更安全；
  // 追求性能可以使用 unique_ptr
  std::vector<Node<K, V>*> forward_;

  Node(K k, V v, int level) : key_(k), value_(v), node_level_(level) {
    // 初始化 forward 数组，大小为 level + 1，全部置空
    this->forward_.resize(level + 1, nullptr);
  }

  ~Node() {}  // vector 会自动释放内存，无需手动 delete []
};