/**
 * @file service_worker.h
 * @author zsy (974483053@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-08-31
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "service/service.h"

/**
 * @brief ServiceWorker 运行时，从全局服务队列中取出服务，然后再处理服务中
 * processing_num 条消息，处理完后若服务仍有未处理消息则重新插入全局服务队列中。
 *
 */
class ServiceWorker {
 public:
  // 编号
  int id_;
  /*
  每次处理多少条消息数量
  值不是越大越好，若工作线程每次处理的消息数量太多，会导致全局队列里的服务得不到及时理。过大的
  processing_num_ 会导致 worker
  线程一直在处理某几个服务的消息，那些在全局队列里等待的服务将得不到及时的处理，有较高的延迟。设置
  processing_num_
  按指数增大，让低编号的工作线程更关注延迟性，高编号的工作线程更关注效率，达到总体的平衡。
  */
  int processing_num_;

 public:
  // 线程函数
  void operator()();

 private:
  // 判断服务是否还有未处理的消息，如果有，把它重新放回全局队列中，等待下一次处理
  void CheckAndPutGlobal(shared_ptr<Service> service);
};