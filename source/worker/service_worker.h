/*
 * @Author: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
 * @Date: 2023-05-24 15:26:45
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git install git && error: git
 * config user.email & please set dead value or install git & please set dead
 * value or install git
 * @LastEditTime: 2023-05-24 16:27:54
 * @FilePath: /XianNet/include/worker.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "service/service.h"

class ServiceWorker {
 public:
  // 编号
  int id_;
  /*
  每次处理多少条消息数量
  但也不是 processing_num_ 越大越好，若工作线程每次处理的消息数量太多，会导致全局队列里的服务得不到及时处理。过大的 processing_num_ 会导致 worker 线程一直在处理某几个服务的消息，那些在全局队列里等待的服务将得不到及时的处理，有较高的延迟。我们让 processing_num_ 按指数增大，让低编号的工作线程更关注延迟性，高编号的工作线程更关注效率，达到总体的平衡。
  */
  int processing_num_;
 public:
  // 线程函数
  void operator()();
 private:
  // 判断服务是否还有未处理的消息，如果有，把它重新放回全局队列中，等待下一次处理
  void CheckAndPutGlobal(shared_ptr<Service> service);
};