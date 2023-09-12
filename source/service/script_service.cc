/**
 * @file script_service.cc
 * @author zsy (974483053@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "message/message.h"
#include "script_service.h"
#include "utility/logger.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-promise.h"
#include "v8-value.h"
#include "xian_net.h"
using namespace std;

ScriptService::ScriptService(const Isolate::CreateParams& create_params,
                             string name)
    : isolate_(Isolate::New(create_params)), BaseService(name) {
  Info("新建 {} 服务", name);

  isolate_->SetStackLimit(100);
  Isolate::Scope isolate_scope(isolate_);
  // Create a stack-allocated handle scope.
  HandleScope handle_scope(isolate_);

  Local<ObjectTemplate> global_template = ObjectTemplate::New(isolate_);

  // 创建脚本运行时可以使用的 c++ 函数
  CreateJsRuntimeEnvironment(global_template);

  // Create a new context.
  Local<Context> context = Context::New(isolate_, nullptr, global_template);

  context->Global()
      ->Set(context, String::NewFromUtf8Literal(isolate_, "service_id"),
            Integer::NewFromUnsigned(isolate_, id_))
      .Check();
  context->Global()
      ->Set(context, ToV8String("service_name"), ToV8String(name_))
      .Check();
  string file_path = "service/";
  file_path.append(name).append(".js");
  auto source_text = GetSourceText(file_path);

  ScriptOrigin origin(isolate_, ToV8String("main"), 0, 0, true, -1,
                      Local<Value>(), false, false, true);
  ScriptCompiler::Source source(source_text, origin, nullptr);

  // Enter the context for compiling and running the hello world script.
  Context::Scope context_scope(context);

  Local<Module> module =
      ScriptCompiler::CompileModule(isolate_, &source).ToLocalChecked();

  auto flag = module->InstantiateModule(
      context,
      [](Local<Context> context, Local<String> specifier,
         Local<FixedArray> import_assertions,
         Local<Module> referrer) -> MaybeLocal<Module> {
        // 这里返回实例化以后的依赖模块
        return MaybeLocal<Module>();
      });
  if (!flag.FromMaybe(false)) {
    Error("模组实例化异常");
    return;
  }

  Local<Value> module_result;
  if (!module->Evaluate(context).ToLocal(&module_result)) {
    // once again, if you have a TryCatch, use it here.
    Error("模块运行结果:{}", ToString(module_result.As<Promise>()->Result()));
  }

  // 创建一个持久的上下文
  persistent_context_.Reset(isolate_, context);

  // 获取所有函数
  Local<Object> exports = module->GetModuleNamespace().As<Object>();
  v8::Local<v8::Array> keys =
      exports->GetOwnPropertyNames(context).ToLocalChecked();

  for (uint32_t i = 0; i < keys->Length(); ++i) {
    v8::Local<v8::Value> key = keys->Get(context, i).ToLocalChecked();
    v8::Local<v8::Value> value = exports->Get(context, key).ToLocalChecked();

    if (value->IsFunction()) {
      js_function_map_.emplace(ToString(key), Local<Function>::Cast(value));
    }
  }
}

ScriptService::~ScriptService() {
  // Dispose the isolate and tear down V8.
  isolate_->Dispose();
}

void ScriptService::OnInitialization() {
  Info("Service {} 开始初始化", id_);
  // 运行 OnInit 函数
  ExecuteJsFunction("OnInit");
}

void ScriptService::OnExit() {
  cout << "Service [" << id_ << "] OnExit" << endl;
  ExecuteJsFunction("OnExit");
}

void ScriptService::ProcessServiceMessage(ServiceMessage* service_message) {
  ExecuteJsFunction(service_message->function_name_, service_message->content_);
}

Local<String> ScriptService::GetSourceText(const string& file_path) {
  string source_file_string;
  ifstream input_file_stream(file_path);
  if (input_file_stream.is_open()) {
    string line;
    while (getline(input_file_stream, line)) {
      source_file_string = source_file_string + line + "\n";
    }
  } else {
    Error("脚本文件 {} 读取失败", name_);
  }
  input_file_stream.close();
  return ToV8String(source_file_string);
}

template <int N>
Local<String> ScriptService::ToV8String(const char (&str)[N]) {
  return ToV8String(isolate_, str);
}
template <int N>
Local<String> ScriptService::ToV8String(Isolate* isolate,
                                        const char (&str)[N]) {
  return String::NewFromUtf8Literal(isolate, str);
}

Local<String> ScriptService::ToV8String(Isolate* isolate, const string& str) {
  return String::NewFromUtf8(isolate, str.data()).ToLocalChecked();
}
Local<String> ScriptService::ToV8String(const string& str) {
  return String::NewFromUtf8(isolate_, str.data()).ToLocalChecked();
}

std::string ScriptService::ToString(Isolate* isolate,
                                    const Local<Value>& value) {
  return *String::Utf8Value(isolate, value);
}
std::string ScriptService::ToString(const Local<Value>& value) {
  return *String::Utf8Value(isolate_, value);
}

Local<Value> ScriptService::GetGlobalValue(Isolate* isolate,
                                           const string& key) {
  auto context = isolate->GetCurrentContext();
  return context->Global()
      ->Get(context, ToV8String(isolate, key))
      .ToLocalChecked();
}

v8::Local<Value> ScriptService::ExecuteJsFunction(const string& function_name,
                                                  const string& content) {
  auto function_iterator = js_function_map_.find(function_name);
  Local<Value> result;
  // 如果未找到该函数则直接返回
  if (function_iterator == js_function_map_.end()) {
    Warning("TS函数 {} 不存在", function_name);
    return result;
  }
  Isolate::Scope isolate_scope(isolate_);
  HandleScope handle_scope(isolate_);
  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(isolate_, persistent_context_);
  Context::Scope context_scope(context);
  auto function = function_iterator->second;

  Info("模块 {} 开始执行函数 {}", name_, function_name);
  // 调用函数
  TryCatch try_catch(isolate_);
  int argc = content.length() == 0 ? 0 : 1;
  Local<Value> argv = ToV8String(content);
  if (!function->Call(context, context->Global(), argc, &argv)
           .ToLocal(&result)) {
    Error("模块 {} 方法 {} 执行失败，原因 {}", name_,
          ToString(function->GetName()), ToString(try_catch.Message()->Get()));
  }
  return result;
}

void ScriptService::SetLogFunctionToNamespace(
    Local<ObjectTemplate> xian_net_namespace, const string& log_function_name) {
  xian_net_namespace->Set(
      isolate_, log_function_name.c_str(),
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            JsDebug(
                ToString(info.GetIsolate(), info[0]),
                ToString(info.GetIsolate(),
                         GetGlobalValue(info.GetIsolate(), "service_name")));
          }));
}

void ScriptService::CreateJsRuntimeEnvironment(
    const Local<ObjectTemplate>& global_template) {
  auto xian_net_namespace = ObjectTemplate::New(isolate_);

  SetLogFunctionToNamespace(xian_net_namespace, "debug");
  SetLogFunctionToNamespace(xian_net_namespace, "info");
  SetLogFunctionToNamespace(xian_net_namespace, "warning");
  SetLogFunctionToNamespace(xian_net_namespace, "error");

  xian_net_namespace->Set(
      isolate_, "newService",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            auto service_id = XianNet::GetInstance().NewScriptService(
                ToString(info.GetIsolate(), info[0]));
            info.GetReturnValue().Set(service_id);
          }));

  xian_net_namespace->Set(
      isolate_, "send",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            auto isolate = info.GetIsolate();
            auto context = isolate->GetCurrentContext();
            uint32_t service_id = GetGlobalValue(isolate, "service_id")
                                      ->Uint32Value(context)
                                      .FromMaybe(0);
            int32_t target_service_id =
                info[0]->Uint32Value(context).FromMaybe(0);
            string function_name = ToString(info.GetIsolate(), info[1]);
            string string_message = ToString(info.GetIsolate(), info[2]);
            auto message = new ServiceMessage();
            message->source_ = service_id;
            message->function_name_ = string(function_name);
            // message->buff = string(*string_message).data()
            XianNet::GetInstance().SendMessage(target_service_id, message);
          }));
  global_template->Set(isolate_, "XianNet", xian_net_namespace);
}