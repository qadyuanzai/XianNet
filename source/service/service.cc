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
