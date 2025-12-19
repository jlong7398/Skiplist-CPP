/**
 * stress_test.cpp - SkipList 多线程压力测试
 *
 * 测试内容：
 * 1. 多线程并发插入性能
 * 2. 多线程并发读取性能
 */
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "../include/SkipList.h"

// 测试配置
constexpr int NUM_THREADS = 4;      // 线程数
constexpr int TEST_COUNT = 100000;  // 总操作数

// 全局跳表实例
SkipList<int, std::string> skipList;

/**
 * 插入元素的工作函数
 * @param thread_id 线程ID
 */
void insertElement(int thread_id) {
  std::cout << "Insert thread " << thread_id << " started" << std::endl;

  int ops_per_thread = TEST_COUNT / NUM_THREADS;
  for (int count = 0; count < ops_per_thread; ++count) {
    skipList.insert_element(rand() % TEST_COUNT, "value");
  }
}

/**
 * 查询元素的工作函数
 * @param thread_id 线程ID
 */
void getElement(int thread_id) {
  std::cout << "Get thread " << thread_id << " started" << std::endl;

  int ops_per_thread = TEST_COUNT / NUM_THREADS;
  std::string value;  // 用于接收查询结果

  for (int count = 0; count < ops_per_thread; ++count) {
    skipList.search_element(rand() % TEST_COUNT, value);
  }
}

int main() {
  srand(static_cast<unsigned>(time(nullptr)));

  // ========== 插入性能测试 ==========
  {
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    auto start = std::chrono::high_resolution_clock::now();

    // 创建并启动所有插入线程
    for (int i = 0; i < NUM_THREADS; ++i) {
      std::cout << "main() : creating insert thread " << i << std::endl;
      threads.emplace_back(insertElement, i);
    }

    // 等待所有线程完成
    for (auto& t : threads) {
      t.join();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "\n[Insert Test] " << TEST_COUNT << " operations with "
              << NUM_THREADS << " threads" << std::endl;
    std::cout << "Total elapsed: " << elapsed.count() << " seconds"
              << std::endl;
    std::cout << "QPS: " << static_cast<int>(TEST_COUNT / elapsed.count())
              << " ops/sec\n"
              << std::endl;
  }

  // ========== 查询性能测试 ==========
  {
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    auto start = std::chrono::high_resolution_clock::now();

    // 创建并启动所有查询线程
    for (int i = 0; i < NUM_THREADS; ++i) {
      std::cout << "main() : creating get thread " << i << std::endl;
      threads.emplace_back(getElement, i);
    }

    // 等待所有线程完成
    for (auto& t : threads) {
      t.join();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "\n[Get Test] " << TEST_COUNT << " operations with "
              << NUM_THREADS << " threads" << std::endl;
    std::cout << "Total elapsed: " << elapsed.count() << " seconds"
              << std::endl;
    std::cout << "QPS: " << static_cast<int>(TEST_COUNT / elapsed.count())
              << " ops/sec\n"
              << std::endl;
  }

  return 0;
}
