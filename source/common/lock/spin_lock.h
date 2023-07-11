#pragma once
#include <thread>

class SpinLock
{
private:
  pthread_spinlock_t lock_;
public:
  SpinLock() { pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE); }
  ~SpinLock() { pthread_spin_destroy(&lock_); }
  void Lock() { pthread_spin_lock(&lock_); }
  void Unlock(){ pthread_spin_unlock(&lock_); }
};