#pragma once

#include <queue>

#include "common/lock/spin_lock.h"

using namespace std;

template <typename T>
class SpinlockObject : public SpinLock {
 private:
  T object_;

 public:
  SpinlockObject() {}
  SpinlockObject(const T& object) : object_(object) {}
  T GetValue() {
    Lock();
    T tmp = object_;
    Unlock();
    return tmp;
  }
  void SetValue(const T& value) {
    Lock();
    object_ = value;
    Unlock();
  }

  T SetCurrentAndGetPrevious(const T& value) {
    Lock();
    T tmp = object_;
    object_ = value;
    Unlock();
    return tmp;
  }
};
