#pragma once

#include "common/thread_safe_container/spinlock_object.h"
#include "common/thread_safe_container/spinlock_queue.h"
#include "message/message.h"

using namespace std;

class BaseService {
 public:
  uint32_t id_;
  string name_;
  bool is_existing_ = false;  // 是否正在退出
  // 用线程安全的容器，线程安全地设置is_in_global_
  // 标记该服务是否在全局队列，true:表示在队列中，或正在处理
  SpinlockObject<bool> is_in_global_queue_ = SpinlockObject<bool>(false);

 public:
  BaseService(const string& name);
  // 创建服务后触发
  virtual void OnInitialization() = 0;
  // 退出服务时触发
  virtual void OnExit() = 0;

  void PushMessage(BaseMessage* message);

  // 处理N条消息，返回值代表是否处理
  void ProcessMessages(int processing_num);
  // 判断该服务中的消息队列是否为空
  bool IsMessageQueueEmpty();

 protected:
  SpinlockQueue<BaseMessage*> message_queue_;

 protected:
  // 执行服务消息
  virtual void ProcessServiceMessage(ServiceMessage* message) {}
  // 执行新连接消息
  virtual void ProcessSocketAcceptMessage(SocketAcceptMessage* message) {}
  // 执行读写消息到socket
  virtual void ProcessSocketRwMessage(SocketRWMessage* message) {}
};
