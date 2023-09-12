/**
 * @file socket_worker.cc
 * @author zsy (974483053@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <iostream>

#include "socket_worker.h"
#include "utility/logger.h"
#include "xian_net.h"
using namespace std;

SocketWorker::SocketWorker() {
  Info("SocketWorker 初始化");
  /*
  epoll_Create1() 用于创建一个epoll实例，并返回代表该实例的文件描述符，epoll_Create1 支持传入一个可用来修改系统调用行为的flags参数，目前支持一个flag标志：EPOLL_CLOEXEC, 使得内核在新的文件描述符上启动执行即关闭标志。
  当创建好epoll句柄后，它就是会占用一个fd值，在linux下如果查看/proc/进程id/fd/，是能够看到这个fd的，所以在使用完epoll后，必须调用close()关闭，否则可能导致fd被耗尽。
  */
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
}
SocketWorker::~SocketWorker() { close(epoll_fd_); }

void SocketWorker::operator()() {
  while (true) {
    //阻塞等待
    const int EVENT_SIZE = 64;
    struct epoll_event m_epoll_events[EVENT_SIZE];
    /*
    通过 epoll_wait 可以多次监听同一个 fd 集合，只返回可读写那部分
    假如有5个就绪事件返回，即 event_count = 5，那么返回的就绪事件信息就会保存在 m_epoll_events 数组的前5个位置，即 m_epoll_events[0] -- m_epoll_events[4]
    */
    int event_count = epoll_wait(epoll_fd_, m_epoll_events, EVENT_SIZE, -1);
    //取得事件
    for (int i = 0; i < event_count; i++) {
      epoll_event m_epoll_event = m_epoll_events[i];  //当前要处理的事件
      OnEvent(m_epoll_event);
    }
  }
}

void SocketWorker::AddEvent(int fd) {
  Info("AddEvent fd: {}", fd);
  // 添加到 epoll 对象
  epoll_event m_epoll_event;
  /*
  events代表要监听的时间类型,可以是以下几个宏的集合：
  EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
  EPOLLOUT：表示对应的文件描述符可以写；
  EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
  EPOLLERR：表示对应的文件描述符发生错误；
  EPOLLHUP：表示对应的文件描述符被挂断；
  EPOLLET： 将EPOLL设为边缘触发(Edge Triggered：上升沿/下降沿)模式，即缓冲区数据从有(无)到无(有)。这是相对于水平触发(Level Triggered：高、低电平触发).使用水平触发模式时，如果没有一次性完成读写操作，那么下次调用epoll_wait时，操作系统还会发出通知；如果使用边缘触发模式，那么操作系统只会通知一次。
  */
  m_epoll_event.events = EPOLLIN | EPOLLET;
  m_epoll_event.data.fd = fd;
  /*
  epoll_ctl
  【作用】：
  向epoll对象中添加/删除/修改需要监听的I/O事件。
  【入参】：
  int epfd：创建的epoll句柄
  int op：表示操作类型。有三个宏表示：
    EPOLL_CTL_ADD：注册新的fd到epfd中；
    EPOLL_CTL_MOD：修改已经注册的fd的监听事件；
    EPOLL_CTL_DEL：从epfd中删除一个fd。
  int fd：要监听的I/O事件（socket、实名管道）的对应的fd
  struct epoll_event *event：表示要监听的事件类型（读写等）
  【返回值】：
  如果操作成功，返回值大于等于0，失败返回值小于0
  */
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &m_epoll_event) < 0) {
    Error("AddEvent epoll_ctl Fail:{}", strerror(errno));
  }
}

void SocketWorker::RemoveEvent(int fd) {
  Info("RemoveEvent fd: {}", fd);
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
}

void SocketWorker::ModifyEvent(int socket_fd, bool can_write) {
  Info("ModifyEvent fd: {}, epoll_out: {}", socket_fd, can_write);
  struct epoll_event m_epoll_event;
  m_epoll_event.data.fd = socket_fd;
  if (can_write) {
    m_epoll_event.events = EPOLLIN | EPOLLET | EPOLLOUT;
  } else {
    m_epoll_event.events = EPOLLIN | EPOLLET;
  }
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket_fd, &m_epoll_event);
}

// 处理事件
void SocketWorker::OnEvent(epoll_event m_epoll_event) {
  int socket_fd = m_epoll_event.data.fd;
  // 根据 socket_fd 从服务器内置的连接集合中获取当前事件对应的连接对象
  auto connection = XianNet::GetInstance().GetConnection(socket_fd);
  if (connection == nullptr) {
    Error("connection为空");
    return;
  }
  // 事件类型
  bool can_read = m_epoll_event.events & EPOLLIN;
  bool can_write = m_epoll_event.events & EPOLLOUT;
  bool has_error = m_epoll_event.events & EPOLLERR;
  // 接收到监听socket事件发生时
  if (connection->type_ == Connection::TYPE::LISTEN) {
    if (can_read) {
      OnAccept(connection);
    }
  }
  //普通socket
  else {
    if (can_read || can_write) {
      OnRW(connection, can_read, can_write);
    }
    if (has_error) {
      cout << "OnError fd:" << connection->fd_ << endl;
    }
  }
}

void SocketWorker::OnAccept(shared_ptr<Connection> connection) {
  Info("OnAccept fd: {}", connection->fd_);
  //步骤1: accept, 返回与新客户端连接创建的 socket_fd
  int client_fd = accept(connection->fd_, NULL, NULL);
  if (client_fd < 0) {
    Error("accept erro");
  }
  //步骤2: 设置非阻塞
  fcntl(client_fd, F_SETFL, O_NONBLOCK);
  //步骤3: 添加连接对象到连接集合中
  XianNet::GetInstance().AddConnection(client_fd, connection->service_id_,
                                       Connection::TYPE::CLIENT);
  //步骤4: 添加到epoll监听列表
  AddEvent(client_fd);
  //步骤5: 通知服务已接受到 socket 新连接
  auto message = new SocketAcceptMessage();
  message->type_ = BaseMessage::TYPE::SOCKET_ACCEPT;
  message->listen_fd_ = connection->fd_;
  message->client_fd_ = client_fd;
  XianNet::GetInstance().SendMessage(connection->service_id_, message);
}

void SocketWorker::OnRW(shared_ptr<Connection> connection, bool can_read,
                        bool can_write) {
  Info("接收到客户端 fd: {} 的 event r/w 事件", connection->fd_);
  auto message = new SocketRWMessage();
  message->type_ = BaseMessage::TYPE::SOCKET_RW;
  message->fd_ = connection->fd_;
  message->can_read_ = can_read;
  message->can_write_ = can_write;
  XianNet::GetInstance().SendMessage(connection->service_id_, message);
}

/*
epoll: event poll 事件轮询
epoll用法详解与编程实例 https://blog.csdn.net/weixin_43354152/article/details/127245783
Epoll的简介及原理 https://zhuanlan.zhihu.com/p/56486633
socket的accept函数解析 https://www.cnblogs.com/langren1992/p/5101380.html
 */