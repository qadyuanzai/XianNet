#pragma once

#include <unordered_map>
#include <vector>

#include "utility/config.h"
#include "service/service.h"
#include "worker/service_worker.h"
#include "worker/socket_worker.h"
#include "service/service_map.h"
#include "common/thread_safe_container/spinlock_queue.h"
#include "common/lock/condition_variable.h"
#include "common/lock/mutex_lock.h"
#include "common/network/connection.h"

class XianNet {
 public:
  static XianNet& GetInstance();
  // 新增服务
  uint32_t NewService(shared_ptr<string> type);

  // 主线程调用该方法后会开启线程阻塞，阻塞调用该方法的线程（主线程），只有 ServiceWorker 线程开始执行服务
  void Wait();
  // 取出“有消息待处理的服务”
  shared_ptr<Service> PopGlobalServiceQueue();
  bool CheckAndPushGlobalServiceQueue(shared_ptr<Service> service);
  // 发送消息
  void SendMessage(uint32_t target_service_id, shared_ptr<BaseMessage> message);
  // 仅供测试用
  shared_ptr<BaseMessage> MakeMsg(uint32_t source, char* buff, int len);
  /**
   * @brief 阻塞工作线程（仅工作线程调用）
   */
  void BlockWorker();
  // 唤醒工作线程
  void CheckAndWeakUpWorker();

  //网络连接操作接口
  int Listen(uint32_t port, uint32_t serviceId);
  void CloseConn(uint32_t fd);
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
  SpinlockQueue<shared_ptr<Service>> global_service_queue_;

  // 用于阻塞以及唤醒服务线程
  ConditionVariable thread_sleep_condition_;
  MutexLock mutex_lock_;
  int sleep_count_ = 0;

  // Socket线程
  SocketWorker* socket_worker_;
  thread* socket_worker_thread_;
  
  // Conn列表
  RwlockUnorderedMap<uint32_t, shared_ptr<Connection>> connection_map_;
 private:
  XianNet();
  XianNet(const XianNet&);
  XianNet& operator=(const XianNet&);
  // 删除服务，仅限服务自己调用，因为在多线程环境下，需要很谨慎地对待删除操作。 
  // XianNet 系统只能由服务在自己的消息处理函数中调用 KillService 删除自己，
  // 这是因为我们没有对 OnExit 和 isExiting 方法加锁，它们不具备线程安全性。
  void KillService(uint32_t id);
  // 获取服务
  shared_ptr<Service> GetService(uint32_t id);

  void StartServiceWorker();
  //开启Socket线程
  void StartSocketWorker();

  // 增删查Conn
  int AddConnection(int fd, uint32_t id, Connection::TYPE type);
  shared_ptr<Connection> GetConnection(int fd);
  bool RemoveConnection(int fd);
};