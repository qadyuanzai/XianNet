#pragma once
#include <thread>


class ConditionVariable
{
private:
  pthread_cond_t condition_variable_;
public:
  ConditionVariable() { pthread_cond_init(&condition_variable_, nullptr); }
  ~ConditionVariable() { pthread_cond_destroy(&condition_variable_); }

  /**
   * @brief 只要到这个函数，就发生阻塞，直到使用 pthread_cond_signal 或者 pthread_cond_broadcast 给条件变量发送信号，此时该线程才继续运行 https://blog.csdn.net/qq_36763031/article/details/118524028
   * @param mutex 
   */
  void Wait(pthread_mutex_t& mutex) { pthread_cond_wait(&condition_variable_, &mutex); }
  void Signal() { pthread_cond_signal(&condition_variable_); }
};