#include "socket_worker.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
using namespace std;

void SocketWorker::Init() {
  cout << "SocketWorker Init" << endl;
  // 创建epoll对象
  epoll_fd_ = epoll_create(
      1024);  // 返回值：非负数表示成功创建的epoll对象的描述符，-1表示创建失败
  assert(epoll_fd_ > 0);
}

void SocketWorker::operator()() {
  while(true) {
    //阻塞等待
    const int EVENT_SIZE = 64;
    struct epoll_event events[EVENT_SIZE];
    int eventCount = epoll_wait(epoll_fd_ , events, EVENT_SIZE, -1);
    //取得事件

    for (int i=0; i<eventCount; i++) {
      epoll_event ev = events[i]; //当前要处理的事件
      OnEvent(ev);
    }
  }
}

void SocketWorker::AddEvent(int fd) {
  cout << "AddEvent fd " << fd << endl;
  // 添加到 epoll 对象
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
    cout << "AddEvent epoll_ctl Fail:" << strerror(errno) << endl;
  }
}

void SocketWorker::RemoveEvent(int fd) {
  cout << "RemoveEvent fd " << fd << endl;
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
}

void SocketWorker::ModifyEvent(int fd, bool epoll_out) {
  cout << "ModifyEvent fd " << fd << " " << epoll_out << endl;
  struct epoll_event ev;
  ev.data.fd = fd;

  if (epoll_out) {
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
  } else {
    ev.events = EPOLLIN | EPOLLET;
  }
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

void SocketWorker::OnEvent(epoll_event event) {
  cout << "OnEvent" << endl;
}
