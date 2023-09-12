/**
 * @file xian_net.h
 * @author zsy (974483053@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <unordered_map>
#include <vector>

#include "common/lock/condition_variable.h"
#include "common/lock/mutex_lock.h"
#include "common/network/connection.h"
#include "common/thread_safe_container/spinlock_queue.h"
#include "lib/v8/include/v8.h"
#include "message/message.h"
#include "service/base_service.h"
#include "service/service_map.h"
#include "utility/config.h"
#include "worker/service_worker.h"
#include "worker/socket_worker.h"

class XianNet {
 public:
  static XianNet& GetInstance();
  void Init();

  // 新增服务
  uint32_t NewService(BaseService* service);
  uint32_t NewScriptService(const string& name);

  // 主线程调用该方法后会开启线程阻塞，阻塞调用该方法的线程（主线程），只有
  // ServiceWorker 线程开始执行服务
  void Wait();
  // 取出“有消息待处理的服务”
  BaseService* PopGlobalServiceQueue();
  bool CheckAndPushGlobalServiceQueue(BaseService* service);
  // 发送消息
  void SendMessage(uint32_t target_service_id, BaseMessage* message);

  /**
   * @brief 阻塞工作线程（仅工作线程调用）
   */
  void BlockWorker();
  // 唤醒工作线程
  void CheckAndWeakUpWorker();

  //网络连接操作接口
  int Listen(uint32_t port, uint32_t service_id);
  void CloseConnection(uint32_t socket_fd);

  // 增删查Conn
  int AddConnection(int socket_fd, uint32_t service_id, Connection::TYPE type);
  shared_ptr<Connection> GetConnection(int socket_fd);
  bool RemoveConnection(int socket_fd);
  //对外Event接口
  void ModifyEvent(int fd, bool epollOut);

 private:
  // 服务列表
  ServiceMap service_map_;
  // 最大id
  uint32_t max_id_ = 0;
  // 配置文件
  Config& config_;
  vector<ServiceWorker*> service_worker_vector_;
  vector<thread*> service_worker_thread_vector_;
  // 把“有消息待处理的服务”放进全局队列，工作线程会不断地从全局队列中取出服务，处理它们
  SpinlockQueue<BaseService*> global_service_queue_;

  // 用于阻塞以及唤醒服务线程
  ConditionVariable thread_sleep_condition_;
  MutexLock mutex_lock_;
  int sleep_count_ = 0;

  // Socket线程
  SocketWorker* socket_worker_;
  thread* socket_worker_thread_;

  // Conn列表
  RwlockUnorderedMap<uint32_t, shared_ptr<Connection>> connection_map_;

  std::unique_ptr<v8::Platform> platform_;
  v8::Isolate::CreateParams create_params_;

 private:
  XianNet();
  ~XianNet();
  XianNet(const XianNet&);
  XianNet& operator=(const XianNet&);
  // 删除服务，仅限服务自己调用，因为在多线程环境下，需要很谨慎地对待删除操作。
  // XianNet 系统只能由服务在自己的消息处理函数中调用 KillService 删除自己，
  // 这是因为我们没有对 OnExit 和 isExiting 方法加锁，它们不具备线程安全性。
  void KillService(uint32_t id);
  // 获取服务
  BaseService* GetService(uint32_t id);

  void StartServiceWorker();
  //开启Socket线程
  void StartSocketWorker();
};