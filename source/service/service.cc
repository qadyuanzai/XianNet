/**
 * @file service.cc
 * @author 钝角 (974483053@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-08-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "service.h"

#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "message/message.h"
#include "utility/logger.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-promise.h"
#include "v8-value.h"
#include "xian_net.h"
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

Service::Service(const Isolate::CreateParams& create_params, string name)
    : isolate_(Isolate::New(create_params)), name_(name) {
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

  context->Global()->Set(context,
                         String::NewFromUtf8Literal(isolate_, "service_id"),
                         Integer::NewFromUnsigned(isolate_, id_));
  string file_path = "service/";
  file_path.append(name).append(".js");
  auto source_text = GetSourceText(file_path);

  ScriptOrigin origin(isolate_, ToV8String("main"), 0, 0, true, -1,
                      Local<Value>(), false, false, true);
  ScriptCompiler::Source source(source_text, origin, nullptr);

  // Enter the context for compiling and running the hello world script.
  Context::Scope context_scope(context);

  module_ = ScriptCompiler::CompileModule(isolate_, &source).ToLocalChecked();

  auto flag = module_->InstantiateModule(
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
  if (!module_->Evaluate(context).ToLocal(&module_result)) {
    // once again, if you have a TryCatch, use it here.
    Error("模块运行结果:{}", ToString(module_result.As<Promise>()->Result()));
  }

  // 创建一个持久的上下文
  persistent_context_.Reset(isolate_, context);

  module_on_init_ = InitializeModuleFunction("OnInit");
  ExecuteModuleFunction(module_on_init_);
}

Service::~Service() {
  // Dispose the isolate and tear down V8.
  isolate_->Dispose();
}

void Service::OnInit() {
  Info("Service {} 开始初始化", id_);

  //开启监听
  XianNet::GetInstance().Listen(8002, id_);
}

void Service::OnExit() { cout << "Service [" << id_ << "] OnExit" << endl; }

void Service::PushMessage(shared_ptr<BaseMessage> message) {
  cout << "Service [" << id_ << "] OnPushMessage" << endl;
  message_queue_.Push(message);
}

bool Service::ProcessMessage() {
  auto message = message_queue_.PopFront();
  if (message != nullptr) {
    Info("服务 {} id:{} 处理消息", name_, id_);
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
  Info("服务 {} id： {} 收到消息执行 {} 函数", name_, id_, msg->function_name_);
  Isolate::Scope isolate_scope(isolate_);

  HandleScope handle_scope(isolate_);
  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(isolate_, persistent_context_);

  Context::Scope context_scope(context);

  auto function = InitializeModuleFunction(msg->function_name_);
  ExecuteModuleFunction(function);
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
  wirteBuff[4200000 - 2] = 'e';
  wirteBuff[4200000 - 1] = '\n';
  int r = write(fd, wirteBuff, 4200000);
  cout << "write r:" << r << " " << strerror(errno) << endl;
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
Local<String> Service::GetSourceText(const string& file_path) {
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
Local<Function> Service::InitializeModuleFunction(const string& function_name) {
  Info("模块 {} 开始初始化函数 {}", name_, function_name);

  // 获取模块的导出对象
  Local<Object> exports = module_->GetModuleNamespace().As<Object>();
  // 获取要运行的函数
  Local<Value> functionValue;
  if (!exports->Get(isolate_->GetCurrentContext(), ToV8String(function_name))
           .ToLocal(&functionValue) ||
      !functionValue->IsFunction()) {
    Warning("Function {} not found in module", function_name);
    return Local<Function>();
  }
  return Local<Function>::Cast(functionValue);
}
template <int N>
Local<String> Service::ToV8String(const char (&str)[N]) {
  return String::NewFromUtf8Literal(isolate_, str);
}
Local<String> Service::ToV8String(const string& str) {
  return String::NewFromUtf8(isolate_, str.data()).ToLocalChecked();
}
v8::Local<Value> Service::ExecuteModuleFunction(
    const Local<Function>& function) {
  Info("模块 {} 开始执行函数 {}", name_, ToString(function->GetName()));
  // 调用函数
  auto context = isolate_->GetCurrentContext();
  Local<Value> result;
  TryCatch try_catch(isolate_);
  if (!function->Call(context, context->Global(), 0, nullptr)
           .ToLocal(&result)) {
    Error("模块 {} 方法 {} 执行失败，原因 {}", name_,
          ToString(function->GetName()), ToString(try_catch.Message()->Get()));
  }
  return result;
}
std::string Service::ToString(const Local<Value>& value) {
  return *String::Utf8Value(isolate_, value);
}
void Service::CreateJsRuntimeEnvironment(
    const Local<ObjectTemplate>& global_template) {
  auto xian_net_namespace = ObjectTemplate::New(isolate_);
  xian_net_namespace->Set(
      isolate_, "debug",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            String::Utf8Value value(info.GetIsolate(), info[0]);
            Debug("{}", *value);
          }));
  xian_net_namespace->Set(
      isolate_, "info",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            String::Utf8Value value(info.GetIsolate(), info[0]);
            Info("{}", *value);
          }));
  xian_net_namespace->Set(
      isolate_, "warning",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            String::Utf8Value value(info.GetIsolate(), info[0]);
            Warning("{}", *value);
          }));
  xian_net_namespace->Set(
      isolate_, "error",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            String::Utf8Value value(info.GetIsolate(), info[0]);
            Error("{}", *value);
          }));

  xian_net_namespace->Set(
      isolate_, "newService",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            String::Utf8Value value(info.GetIsolate(), info[0]);
            auto service_id = XianNet::GetInstance().NewService(*value);
            info.GetReturnValue().Set(service_id);
          }));

  xian_net_namespace->Set(
      isolate_, "send",
      FunctionTemplate::New(
          isolate_, [](const FunctionCallbackInfo<Value>& info) {
            auto isolate = info.GetIsolate();
            auto context = isolate->GetCurrentContext();
            uint32_t service_id = context->Global()
                                      ->Get(context, String::NewFromUtf8Literal(
                                                         isolate, "service_id"))
                                      .ToLocalChecked()
                                      ->Uint32Value(context)
                                      .FromMaybe(0);
            int32_t target_service_id =
                info[0]->Uint32Value(context).FromMaybe(0);
            String::Utf8Value function_name(isolate, info[1]);
            String::Utf8Value string_message(isolate, info[2]);
            auto message = make_shared<ServiceMessage>();
            message->source = service_id;
            message->function_name_ = string(*function_name);
            // message->buff = string(*string_message).data()
            XianNet::GetInstance().SendMessage(target_service_id, message);
          }));
  global_template->Set(isolate_, "XianNet", xian_net_namespace);
}