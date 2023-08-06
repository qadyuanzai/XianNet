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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "utility/logger.h"
#include "v8-primitive.h"
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

static void InfoCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() < 1) return;
  Isolate* isolate = info.GetIsolate();
  HandleScope scope(isolate);
  Local<Value> arg = info[0];
  String::Utf8Value value(isolate, arg);
  Info("{}", *value);
}

Service::Service(const v8::Isolate::CreateParams& create_params, string name)
    : isolate_(v8::Isolate::New(create_params)), name_(name) {
  Info("新建 v8 引擎 isolate");
  v8::Isolate::Scope isolate_scope(isolate_);
  // Create a stack-allocated handle scope.
  v8::HandleScope handle_scope(isolate_);

  Local<ObjectTemplate> global = ObjectTemplate::New(isolate_);
  global->Set(isolate_, "Info", FunctionTemplate::New(isolate_, InfoCallback));

  // Create a new context.
  v8::Local<v8::Context> context = v8::Context::New(isolate_, nullptr, global);

  // Enter the context for compiling and running the hello world script.
  v8::Context::Scope context_scope(context);

  auto source_text = GetSourceText("service/main.ts");
  // 编译运行脚本
  v8::Local<v8::Script> script =
      v8::Script::Compile(context, source_text).ToLocalChecked();
  {
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
      v8::TryCatch try_catch(isolate_);
      v8::String::Utf8Value error(isolate_, try_catch.Exception());
      Error("{} 脚本运行失败，错误为：{}", name_, *error);
      return;
    }
  }

  v8::Local<v8::String> function_name =
      v8::String::NewFromUtf8Literal(isolate_, "OnInit");
  v8::Local<v8::Value> process_val;
  if (!context->Global()->Get(context, function_name).ToLocal(&process_val) ||
      !process_val->IsFunction()) {
    Error("没有找到函数 {0}, 或者 {0} 不是一个函数", "OnInit");
    return;
  }
  // v8::Local<v8::Function> function =
  //     v8::Local<v8::Function>::New(isolate_, process_val);
  v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(process_val);

  v8::Local<v8::Value> result;
  if (!function->Call(context, context->Global(), 0, nullptr)
           .ToLocal(&result)) {
    return;
  }
  // Convert the result to an UTF8 string and print it.
  v8::String::Utf8Value utf8(isolate_, result);
  Info("执行 {} 函数的返回值为：{}", "OnInit", *utf8);
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
v8::Local<String> Service::GetSourceText(const string& file_path) {
  string source_file_string;
  ifstream input_file_stream(file_path);
  if (input_file_stream.is_open()) {
    string line;
    while (getline(input_file_stream, line)) {
      source_file_string = source_file_string + line + "\n";
    }
    Info("脚本文件 {} 读取成功， 内容：\n{}", name_, source_file_string);
  } else {
    Error("脚本文件 {} 读取失败", name_);
  }
  input_file_stream.close();

  return v8::String::NewFromUtf8(isolate_, source_file_string.data())
      .ToLocalChecked();
}