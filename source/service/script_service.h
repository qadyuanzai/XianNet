/**
 * @file script_service.h
 * @author zsy (974483053@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <string>

#include "lib/v8/include/v8.h"
#include "message/message.h"
#include "service/base_service.h"

using namespace std;
using namespace v8;

class ScriptService : public BaseService {
 public:
  ScriptService(const Isolate::CreateParams& create_params, string name);
  ~ScriptService();
  // 创建服务后触发
  void OnInitialization() override;
  // 退出服务时触发
  void OnExit() override;

 private:
  Isolate* isolate_;
  // 用于存储初始化后的上下文环境
  Persistent<Context> persistent_context_;
  // 所有在 js 文件中暴露（export）出来的方法
  unordered_map<string, Local<Function>> js_function_map_;

 private:
  void ProcessServiceMessage(ServiceMessage* service_message) override;

  Local<String> GetSourceText(const string& file_path);
  void CreateJsRuntimeEnvironment(const Local<ObjectTemplate>& global_template);

  Local<Value> ExecuteJsFunction(const string& function_name,
                                 const string& message_content = "");

  template <int N>
  static Local<String> ToV8String(Isolate* isolate, const char (&str)[N]);
  template <int N>
  Local<String> ToV8String(const char (&str)[N]);
  static Local<String> ToV8String(Isolate* isolate, const string& str);
  Local<String> ToV8String(const string& str);
  static string ToString(Isolate* isolate, const Local<Value>& value);
  string ToString(const Local<Value>& value);

  static Local<Value> GetGlobalValue(Isolate* isolate, const string& key);

  void SetLogFunctionToNamespace(Local<ObjectTemplate> xian_net_namespace,
                                 const string& log_function_name);
};
