/*
 * @Author: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
 * @Date: 2023-05-23 17:59:09
 * @LastEditors: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git install git && error: git
 * config user.email & please set dead value or install git & please set dead
 * value or install git install git && error: git config user.email & please set
 * dead value or install git & please set dead value or install git install git
 * && error: git config user.email & please set dead value or install git &
 * please set dead value or install git install git
 * && error: git config user.email & please set dead value or install git &
 * please set dead value or install git
 * @LastEditTime: 2023-05-25 11:27:51
 * @FilePath: /XianNet/include/service.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include <queue>
#include <thread>
#include <iostream>

#include "helper/thread_safe_container/spinlock_object.h"
#include "helper/thread_safe_container/spinlock_queue.h"
#include "message/message.h"


using namespace std;

class Service {
 public:
  uint32_t id_;
  shared_ptr<string> type_;
  bool is_existing_ = false;  // 是否正在退出
  // 用线程安全的容器，线程安全地设置is_in_global_
  // 标记该服务是否在全局队列，true:表示在队列中，或正在处理
  SpinlockObject<bool> is_in_global_queue_ = SpinlockObject<bool>(false);

 public:
  // 创建服务后触发
  void OnInit();
  // 退出服务时触发
  void OnExit();
  // 将消息插入服务的消息队列中
  void PushMessage(shared_ptr<BaseMessage> message);
  // 处理N条消息，返回值代表是否处理
  // 调用该函数会将 is_in_global_queue_ 为 false
  void ProcessMessages(int max);
  bool IsMessageQueueEmpty();

 private:
  SpinlockQueue<shared_ptr<BaseMessage>> message_queue_;
  

 private:
  // 取出一条消息
  shared_ptr<BaseMessage> PopMessage();
  // 执行消息
  bool ProcessMessage();
};