#pragma once

#include <unordered_map>
#include "helper/lock/rw_lock.h"
using namespace std;

template<typename K, typename V>
class RwlockUnorderedMap : public RwLock {
 public:
  V Find(K key) {
    LockRead();
    // 在容器中搜索键值等于 k 的元素，如果找到，则返回一个指向该元素的迭代器，否则返回一个指向unordered_map::end的迭代器。
    auto iter = map_.find(key);
    V value = nullptr;
    if (iter != map_.end()) {
      value = iter->second;
    }
    Unlock();
    return value;
  }

  void Emplace(K key, V value) {
    LockWrite();
    // emplace效率比insert高
    map_.emplace(key, value);
    Unlock();
  }

  void Erase(K key) {
    LockWrite();
    map_.erase(key);
    Unlock();
  }
 protected:
  unordered_map<K, V> map_;
};
