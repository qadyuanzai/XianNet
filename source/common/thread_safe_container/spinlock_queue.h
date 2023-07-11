#pragma once

#include <queue>

#include "common/lock/spin_lock.h"

using namespace std;

template <typename T>
class SpinlockQueue : public SpinLock {
 public:
  void Push(const T& t) {
    Lock();
    queue_.push(t);
    Unlock();
  }
  // 会返回 front 对象并从队列中 pop 掉
  // 对象不存在会返回 nullptr
  T PopFront() {
    Lock();
    T t = nullptr;
    if (!queue_.empty()) {
      t = queue_.front();
      queue_.pop();
    }
    Unlock();
    return t;
  }

  bool IsEmpty() {
    Lock();
    bool empty = queue_.empty();
    Unlock();
    return empty;
  }

  int GetSize() {
    return queue_.size();
  }
 protected:
  queue<T> queue_;
};

