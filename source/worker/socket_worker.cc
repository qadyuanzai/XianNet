#include "socket_worker.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "xian_net.h"
#include <fcntl.h>
#include <sys/socket.h>

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


//处理事件
void SocketWorker::OnEvent(epoll_event ev){
  int fd = ev.data.fd;
  auto conn = XianNet::GetInstance().GetConnection(fd);
  if(conn == NULL){
    cout << "OnEvent error, conn == NULL" << endl;
    return;
  }
  //事件类型
  bool isRead = ev.events & EPOLLIN;
  bool isWrite = ev.events & EPOLLOUT;
  bool isError = ev.events & EPOLLERR;
  //监听socket
  if(conn->type_ == Connection::TYPE::LISTEN){
    if(isRead) {
      OnAccept(conn);
    }
  }
  //普通socket
  else {
    if(isRead || isWrite) {
      OnRW(conn, isRead, isWrite);
    }
    if(isError){
      cout << "OnError fd:" << conn->fd_ << endl;
    }
  }
}

void SocketWorker::OnAccept(shared_ptr<Connection> conn) {
    cout << "OnAccept fd:" << conn->fd_ << endl;
    //步骤1：accept
    int clientFd = accept(conn->fd_, NULL, NULL);
    if (clientFd < 0) {
        cout << "accept error" << endl;
    }
    //步骤2：设置非阻塞
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    //步骤3：添加连接对象
    XianNet::GetInstance().AddConnection(clientFd, conn->service_id_, Connection::TYPE::CLIENT);
    //步骤4：添加到epoll监听列表
        struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = clientFd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
        cout << "OnAccept epoll_ctl Fail:" << strerror(errno) << endl;
    }
    //步骤5：通知服务
    auto msg= make_shared<SocketAcceptMessage>();
    msg->type = BaseMessage::TYPE::SOCKET_ACCEPT;
    msg->listen_fd_ = conn->fd_;
    msg->client_fd_ = clientFd;
    XianNet::GetInstance().SendMessage(conn->service_id_, msg);
}

void SocketWorker::OnRW(shared_ptr<Connection> conn, bool r, bool w) {
  cout << "OnRW fd:" << conn->fd_ << endl;
  auto msg= make_shared<SocketRWMessage>();
  msg->type = BaseMessage::TYPE::SOCKET_RW;
  msg->fd_ = conn->fd_;
  msg->is_read_ = r;
  msg->is_write_ = w;
   XianNet::GetInstance().SendMessage(conn->service_id_, msg);
}