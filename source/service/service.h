/*
 * @file service.h
 * @author 钝角 (974483053@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-07-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "common/network/ConnWriter.h"
#include "common/thread_safe_container/spinlock_object.h"
#include "common/thread_safe_container/spinlock_queue.h"
#include "lib/v8/include/v8.h"
#include "message/message.h"
#include "unordered_map"
#include "v8-isolate.h"
#include "v8-value.h"

using namespace std;
using namespace v8;

class Service {
 public:
  uint32_t id_;
  string name_;
  bool is_existing_ = false;  // 是否正在退出
  // 用线程安全的容器，线程安全地设置is_in_global_
  // 标记该服务是否在全局队列，true:表示在队列中，或正在处理
  SpinlockObject<bool> is_in_global_queue_ = SpinlockObject<bool>(false);

 public:
  Service(const Isolate::CreateParams& create_params, string name);
  ~Service();
  // 创建服务后触发
  void OnInitialization();
  // 退出服务时触发
  void OnExit();
  // 将消息插入服务的消息队列中
  void PushMessage(shared_ptr<BaseMessage> message);
  // 处理N条消息，返回值代表是否处理
  // 调用该函数会将 is_in_global_queue_ 为 false
  void ProcessMessages(int processing_num);
  // 判断该服务中的消息队列是否为空
  bool IsMessageQueueEmpty();

 private:
  SpinlockQueue<shared_ptr<BaseMessage>> message_queue_;
  //业务逻辑（仅用于测试）
  unordered_map<int, shared_ptr<ConnWriter>> writers;
  // 可以理解为该服务单独的一个虚拟机
  Isolate* isolate_;
  // 用于存储初始化后的上下文环境
  Persistent<Context> persistent_context_;
  // 所有在 js 文件中暴露（export）出来的方法
  unordered_map<string, Local<Function>> js_function_map_;

 private:
  // 取出一条消息
  shared_ptr<BaseMessage> PopMessage();
  // 执行消息
  bool ProcessMessage();
  void OnServiceMsg(shared_ptr<ServiceMessage> msg);
  void OnAcceptMsg(shared_ptr<SocketAcceptMessage> msg);
  void OnRWMsg(shared_ptr<SocketRWMessage> msg);
  void OnSocketData(int fd, const char* buff, int len);
  void OnSocketWritable(int fd);
  void OnSocketClose(int fd);

  Local<String> GetSourceText(const string& file_path);
  void CreateJsRuntimeEnvironment(const Local<ObjectTemplate>& global_template);

  Local<Value> ExecuteJsFunction(const string& function_name);

  template <int N>
  static Local<String> ToV8String(Isolate* isolate, const char (&str)[N]);
  template <int N>
  Local<String> ToV8String(const char (&str)[N]);
  static Local<String> ToV8String(Isolate* isolate, const string& str);
  Local<String> ToV8String(const string& str);
  static string ToString(Isolate* isolate, const Local<Value>& value);
  string ToString(const Local<Value>& value);

  static Local<Value> GetGlobalValue(Isolate* isolate, const string& key);
};
