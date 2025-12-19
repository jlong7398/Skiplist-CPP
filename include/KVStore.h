// KVStore.h - 基于 SkipList 实现的键值存储类
// include/KVStore.h
#pragma once
// 防止头文件被多次包含
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "SkipList.h"

template <typename K, typename V>
class KVStore {
 private:
  SkipList<K, V>* skip_list_;  // 底层跳表实例指针
  std::string file_path_;      // 持久化文件路径

 public:
  KVStore(const std::string& path) : file_path_(path) {
    // 构造函数：初始化文件路径并创建 SkipList 实例，然后加载已有数据
    skip_list_ = new SkipList<K, V>();
    load();  // 从磁盘加载持久化数据
  }

  ~KVStore() {
    // 析构函数：在对象销毁前将内存中的数据持久化到磁盘
    dump();  // 自动保存数据到磁盘
    delete skip_list_;
  }

  void put(const K& key, const V& value) {
    // 插入或更新键值对
    skip_list_->insert_element(key, value);
  }

  bool get(const K& key, V& value) {
    // 根据键查询对应的值，若不存在返回 false
    return skip_list_->search_element(key, value);
  }

  void del(const K& key) { skip_list_->delete_element(key); }
  // 删除指定键的记录

  void clear() { skip_list_->clear(); }
  // 清空跳表

  // ---------- 持久化相关 ----------
  // 实现保存
  void dump() {
    // 将当前 SkipList 中的所有键值对写入文件
    std::ofstream out_file(file_path_);
    if (!out_file.is_open()) {
      std::cerr << "Error opening file for dump: " << file_path_ << std::endl;
      return;
    }
    // 使用 SkipList 的遍历接口，将每条记录写入文件
    skip_list_->process_all([&](const K& key, const V& value) {
      out_file << key << ":" << value << "\n";
    });
    out_file.close();
  }

  // 实现加载
  // 从文件读取键值对并恢复到 SkipList
  void load() {
    // 从文件读取键值对并恢复到 SkipList
    std::ifstream in_file(file_path_);
    if (!in_file.is_open()) {
      // 文件不存在或无法打开，通常是首次运行，直接返回，保持 KVStore 为空状态
      return;
    }
    // 按行读取内容
    std::string line;
    while (std::getline(in_file, line)) {
      size_t pos = line.find(':');  // 每行为"key:value"
      if (pos != std::string::npos) {
        // 拆分为key和value的字符串
        std::string key_str = line.substr(0, pos);
        std::string value_str = line.substr(pos + 1);

        K key;
        std::stringstream ss(key_str);
        ss >> key;  // 将字符串键转换为实际类型K

        V value;
        // 使用if constexpr在编译时根据V的类型选择不同的解析方式
        // 避免对std::string进行不必要的stringstream解析
        if constexpr (std::is_same<V, std::string>::value) {
          value = value_str;
        } else {
          std::stringstream ss_val(value_str);
          ss_val >> value;  // 将字符串值转换为实际类型V
        }

        skip_list_->insert_element(key, value);  // 将解析出的键值对插入到跳表
      }
    }
    // 读取完毕后关闭文件句柄
    in_file.close();
  }
};