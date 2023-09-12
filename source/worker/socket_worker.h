/**
 * @file socket_worker.h
 * @author zsy (974483053@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-09-09
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <sys/epoll.h>

#include <memory>
#include <thread>

#include "common/network/connection.h"
using namespace std;

class SocketWorker {
 public:
  SocketWorker();
  ~SocketWorker();
  /**
   * @brief 程序循环监听 epoll 事件, epoll 事件发生时, 
   * 
   */
  void operator()();  // 线程函数

  void AddEvent(int fd);
  void RemoveEvent(int fd);

  /**
  * @brief 修改监听事件的方法 (目前只支持修改 socket 事件是否可写)
  *
  * @param socket_fd
  * @param can_write 是否可写
  */
  void ModifyEvent(int socket_fd, bool can_write);

 private:
  int epoll_fd_;

 private:
  /**
   * @brief 从 connection 中获取该事件 fd 对应的 connection, 再根据connection 获取对应的服务 id, 然后对该服务发送对应的消息
   * 目前只有 gateway 服务
   * 
   * @param event 
   */
  void OnEvent(epoll_event event);

  /**
   * @brief OnAccept 事件用于处理服务器接受的新连接，在触发 OnAccept 事件时，服务器将新建立的 socket 连接对象并存于服务器内置的连接集合中.
   *
   * @param connection
   */
  void OnAccept(shared_ptr<Connection> connection);

  // 接收到的 普通 socket 事件是可读/写时, 发送消息(connection)给 service
  void OnRW(shared_ptr<Connection> connection, bool can_read, bool can_write);
};