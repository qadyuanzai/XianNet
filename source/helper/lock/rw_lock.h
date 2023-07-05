#pragma once
#include <thread>

class RwLock
{
private:
  pthread_rwlock_t lock_;
public:
  RwLock() { pthread_rwlock_init(&lock_, nullptr); }
  ~RwLock() { pthread_rwlock_destroy(&lock_); };
  void LockRead() { pthread_rwlock_rdlock(&lock_); }
  void LockWrite() { pthread_rwlock_wrlock(&lock_); }
  void Unlock(){ pthread_rwlock_unlock(&lock_); }
};