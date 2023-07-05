#pragma once
#include <thread>

class MutexLock
{
private:
  pthread_mutex_t mutex_;
public:
  MutexLock() { pthread_mutex_init(&mutex_, nullptr); }
  ~MutexLock() { pthread_mutex_destroy(&mutex_); }
  void Lock() { pthread_mutex_lock(&mutex_); }
  void Unlock(){ pthread_mutex_unlock(&mutex_); }
  pthread_mutex_t& mutex() {return mutex_;}
};