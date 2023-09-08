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

#include <iostream>

#include "socket_worker.h"
#include "utility/logger.h"
#include "xian_net.h"
using namespace std;
/*
epoll: event poll 事件轮询
epoll用法详解与编程实例 https://blog.csdn.net/weixin_43354152/article/details/127245783
Epoll的简介及原理 https://zhuanlan.zhihu.com/p/56486633

 */
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
  EPOLLET： 将EPOLL设为边缘触发(Edge Triggered：上升沿/下降沿)模式，即缓冲区数据从有(无)到无(有)。这是相对于水平触发(Level Triggered：高、低电平触发)：缓冲区数据读没有读完，写没有写满。select()函数只支持水平触发。
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

void SocketWorker::ModifyEvent(int fd, bool epoll_out) {
  Info("ModifyEvent fd: {}, epoll_out: {}", fd, epoll_out);
  struct epoll_event m_epoll_event;
  m_epoll_event.data.fd = fd;
  if (epoll_out) {
    m_epoll_event.events = EPOLLIN | EPOLLET | EPOLLOUT;
  } else {
    m_epoll_event.events = EPOLLIN | EPOLLET;
  }
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &m_epoll_event);
}

//处理事件
void SocketWorker::OnEvent(epoll_event m_epoll_event) {
  int fd = m_epoll_event.data.fd;
  auto connection = XianNet::GetInstance().GetConnection(fd);
  if (connection == NULL) {
    Error("connection为空");
    return;
  }
  //事件类型
  bool isRead = m_epoll_event.events & EPOLLIN;
  bool isWrite = m_epoll_event.events & EPOLLOUT;
  bool isError = m_epoll_event.events & EPOLLERR;
  //监听socket
  if (connection->type_ == Connection::TYPE::LISTEN) {
    if (isRead) {
      OnAccept(connection);
    }
  }
  //普通socket
  else {
    if (isRead || isWrite) {
      OnRW(connection, isRead, isWrite);
    }
    if (isError) {
      cout << "OnError fd:" << connection->fd_ << endl;
    }
  }
}

void SocketWorker::OnAccept(shared_ptr<Connection> connection) {
  Info("OnAccept fd: {}", connection->fd_);
  //步骤1:accept
  int client_fd = accept(connection->fd_, NULL, NULL);
  if (client_fd < 0) {
    cout << "accept error" << endl;
  }
  //步骤2:设置非阻塞
  fcntl(client_fd, F_SETFL, O_NONBLOCK);
  //步骤3:添加连接对象
  XianNet::GetInstance().AddConnection(client_fd, connection->service_id_,
                                       Connection::TYPE::CLIENT);
  //步骤4:添加到epoll监听列表
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = client_fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
    cout << "OnAccept epoll_ctl Fail:" << strerror(errno) << endl;
  }
  //步骤5:通知服务
  auto msg = make_shared<SocketAcceptMessage>();
  msg->type = BaseMessage::TYPE::SOCKET_ACCEPT;
  msg->listen_fd_ = connection->fd_;
  msg->client_fd_ = client_fd;
  XianNet::GetInstance().SendMessage(connection->service_id_, msg);
}

void SocketWorker::OnRW(shared_ptr<Connection> conn, bool r, bool w) {
  cout << "OnRW fd:" << conn->fd_ << endl;
  auto msg = make_shared<SocketRWMessage>();
  msg->type = BaseMessage::TYPE::SOCKET_RW;
  msg->fd_ = conn->fd_;
  msg->is_read_ = r;
  msg->is_write_ = w;
  XianNet::GetInstance().SendMessage(conn->service_id_, msg);
}