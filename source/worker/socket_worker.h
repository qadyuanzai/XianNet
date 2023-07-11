#pragma once
#include <sys/epoll.h>
#include <thread>
#include "common/network/connection.h"
using namespace std;

class SocketWorker {
 public:
  void Init();        // 初始化
  void operator()();  // 线程函数

  void AddEvent(int fd);
  void RemoveEvent(int fd);
  void ModifyEvent(int fd, bool epoll_out);

 private:
  int epoll_fd_;

 private:
  void OnEvent(epoll_event event);
  // void OnAccept(shared_ptr<Connection> connection);
  // void OnRW(shared_ptr<Connection> connection, bool r, bool w);
};