#pragma once
#include "message/message.h"
#include "service/base_service.h"

class GatewayService : public BaseService {
 public:
  GatewayService(const string& name);

 public:
  // 创建服务后触发
  void OnInitialization() override;
  // 退出服务时触发
  void OnExit() override;
  // 将消息插入服务的消息队列中
  void PushMessage(shared_ptr<BaseMessage> message);
  // 处理N条消息，返回值代表是否处理
  // 调用该函数会将 is_in_global_queue_ 为 false
  void ProcessMessages(int processing_num);
  // 判断该服务中的消息队列是否为空
  bool IsMessageQueueEmpty();

 private:
  //
 private:
  // 执行新连接消息
  void ProcessSocketAcceptMessage(SocketAcceptMessage* message) override;
  void ProcessSocketRwMessage(SocketRWMessage* message) override;
  // void OnSocketData(int fd, const char* buff, int len);
  // void OnSocketWritable(int fd);
  // void OnSocketClose(int fd);
  string Read(int fd);
  void Write(int fd, const string& content);
};