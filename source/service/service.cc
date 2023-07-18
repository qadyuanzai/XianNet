/*
 * @Author: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
 * @Date: 2023-05-24 09:58:37
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
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

void Service::OnExit() { 
  cout << "Service [" << id_ << "] OnExit" << endl; 
}

void Service::PushMessage(shared_ptr<BaseMessage> message) {
  cout << "Service [" << id_ << "] OnPushMessage" << endl; 
  message_queue_.Push(message);
}

bool Service::ProcessMessage() { 
  cout << "Service [" << id_ << "] OnProcessMessage" << endl;
  auto message = message_queue_.PopFront();
  if (message != nullptr) {
    //SOCKET_ACCEPT
    if(message->type == BaseMessage::TYPE::SOCKET_ACCEPT) {
        auto m = dynamic_pointer_cast<SocketAcceptMessage>(message);
        cout << "new conn " << m->client_fd_ << endl;
    }
    //SOCKET_RW
    if(message->type == BaseMessage::TYPE::SOCKET_RW) {
        auto m = dynamic_pointer_cast<SocketRWMessage>(message);
        if(m->is_read_) {
            char buff[512];
            int len = read(m->fd_, &buff, 512);
            if(len > 0) {
                char wirteBuff[3] = {'l','p','y'};
                write(m->fd_, &wirteBuff, 3);
            }
            else {
                cout << "close " << m->fd_ << strerror(errno) <<  endl;
                XianNet::GetInstance().CloseConn(m->fd_);
            }
        }
    }
    if(message->type == BaseMessage::TYPE::SERVICE) {
      // 测试用
      auto m = dynamic_pointer_cast<ServiceMessage>(message);
      cout << "[" << id_ <<"] OnMsg " << m->buff << endl;
      usleep(1000000);
      auto msgRet = XianNet::GetInstance().MakeMsg(id_, new char[5] { 'p', 'i', 'n', 'g', '\0' }, 5);
      XianNet::GetInstance().SendMessage(m->source, msgRet);
    }
    return true;
  }
  return false;
}
