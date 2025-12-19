#include <iostream>
#include <string>

#include "KVStore.h"

#define FILE_PATH "./store/dumpFile"

int main() {
  // 使用 KVStore 而不是直接使用 SkipList
  KVStore<int, std::string> kvStore(FILE_PATH);

  // 插入一些数据
  kvStore.put(1, "good");
  kvStore.put(3, "good");
  kvStore.put(7, "study");
  kvStore.put(8, ",");
  kvStore.put(9, "day");
  kvStore.put(19, "day");
  kvStore.put(29, "up");

  std::string value;
  if (kvStore.get(9, value)) {
    std::cout << "Found key 9: " << value << std::endl;
  } else {
    std::cout << "Key 9 not found." << std::endl;
  }

  // 持久化会在析构函数中自动调用
  // 也可以手动调用 kvStore.dump(); 但它是 public 接口吗？是的

  kvStore.del(3);
  kvStore.del(7);

  if (kvStore.get(3, value)) {
    std::cout << "Found key 3: " << value << std::endl;
  } else {
    std::cout << "Key 3 deleted as expected." << std::endl;
  }

  std::cout << "KVStore operations finished." << std::endl;
  return 0;
}
