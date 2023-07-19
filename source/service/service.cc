/*
 * @Author: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
 * @Date: 2023-05-24 09:58:37
 * @LastEditors: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git install git && error: git
 * config user.email & please set dead value or install git & please set dead
 * value or install git
 * @LastEditTime: 2023-05-25 14:43:31
 * @FilePath: /XianNet/src/service.cc
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "service.h"
#include "xian_net.h"
#include <unistd.h>

#include <iostream>
using namespace std;

void Service::ProcessMessages(int max) {
  for (int i = 0; i < max; i++) {
    bool succ = ProcessMessage();
    if (!succ) {
      break;
    }
  }
}

bool Service::IsMessageQueueEmpty() { return message_queue_.IsEmpty(); }

shared_ptr<BaseMessage> Service::PopMessage() {
  cout << "Service [" << id_ << "] OnPopMessage" << endl;
  return message_queue_.PopFront();
}

void Service::OnInit() {
  cout << "Service [" << id_ << "] OnInit" << endl;
  //开启监听
  XianNet::GetInstance().Listen(8002, id_);
}

void Service::OnExit() { cout << "Service [" << id_ << "] OnExit" << endl; }

void Service::PushMessage(shared_ptr<BaseMessage> message) {
  cout << "Service [" << id_ << "] OnPushMessage" << endl;
  message_queue_.Push(message);
}

bool Service::ProcessMessage() {
  cout << "Service [" << id_ << "] OnProcessMessage" << endl;
  auto message = message_queue_.PopFront();
  if (message != nullptr) {
    // SERVICE
    if (message->type == BaseMessage::TYPE::SERVICE) {
      auto m = dynamic_pointer_cast<ServiceMessage>(message);
      OnServiceMsg(m);
    }
    // SOCKET_ACCEPT
    else if (message->type == BaseMessage::TYPE::SOCKET_ACCEPT) {
      auto m = dynamic_pointer_cast<SocketAcceptMessage>(message);
      OnAcceptMsg(m);
    }
    // SOCKET_RW
    else if (message->type == BaseMessage::TYPE::SOCKET_RW) {
      auto m = dynamic_pointer_cast<SocketRWMessage>(message);
      OnRWMsg(m);
    }
    return true;
  }
  return false;
}

//收到其他服务发来的消息
void Service::OnServiceMsg(shared_ptr<ServiceMessage> msg) {
  cout << "OnServiceMsg " << endl;
}

//新连接
void Service::OnAcceptMsg(shared_ptr<SocketAcceptMessage> msg) {
  cout << "OnAcceptMsg " << msg->client_fd_ << endl;
  auto w = make_shared<ConnWriter>();
  w->fd = msg->client_fd_;
  writers.emplace(msg->client_fd_, w);
}

//套接字可读可写
void Service::OnRWMsg(shared_ptr<SocketRWMessage> msg) {
  int fd = msg->fd_;
  //可读
  if (msg->is_read_) {
    const int BUFFSIZE = 512;
    char buff[BUFFSIZE];
    int len = 0;
    do {
      len = read(fd, &buff, BUFFSIZE);
      if (len > 0) {
        OnSocketData(fd, buff, len);
      }
    } while (len == BUFFSIZE);

    if (len <= 0 && errno != EAGAIN) {
      if (XianNet::GetInstance().GetConnection(fd)) {
        OnSocketClose(fd);
        XianNet::GetInstance().CloseConn(fd);
      }
    }
  }
  //可写（注意没有else）
  if (msg->is_write_) {
    if (XianNet::GetInstance().GetConnection(fd)) {
      OnSocketWritable(fd);
    }
  }
}

// //收到客户端数据
void Service::OnSocketData(int fd, const char* buff, int len) {
    cout << "OnSocketData" << fd << " buff: " << buff << endl;
   //用ConnWriter发送大量数据
   char* wirteBuff = new char[4200000];
   wirteBuff[4200000-2] = 'e';
   wirteBuff[4200000-1] = '\n';
   int r = write(fd, wirteBuff, 4200000); 
   cout << "write r:" << r <<  " " << strerror(errno) <<  endl;
   auto w = writers[fd];
   w->EntireWrite(shared_ptr<char>(wirteBuff), 4200000);
   w->LingerClose();
}

//套接字可写
void Service::OnSocketWritable(int fd) {
  cout << "OnSocketWritable " << fd << endl;
      auto w = writers[fd];
    w->OnWriteable();
}

//关闭连接前
void Service::OnSocketClose(int fd) {
  cout << "OnSocketClose " << fd << endl;
  writers.erase(fd);
}
