/**
 * @file message.h
 * @author zsy (974483053@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <memory>
using namespace std;

class BaseMessage {
 public:
  enum class TYPE {
    SERVICE,
    SOCKET_ACCEPT,
    SOCKET_RW,
  };
  TYPE type_;
  // 用于检测内存泄漏
  char load[100000000]{};
};

struct ServiceMessage : BaseMessage {
  uint32_t source_;       // 消息发送方
  string function_name_;  // 调用方法名
  // TODO:换成指针
  string content_;  // 消息内容
};

// 有新连接
struct SocketAcceptMessage : BaseMessage {
  // 监听套接字的描述符
  int listen_fd_;
  // 是新连入客户端的套接字描述符
  int client_fd_;
};

//可读可写
class SocketRWMessage : public BaseMessage {
 public:
  int fd_;
  bool can_read_ = false;
  bool can_write_ = false;
};