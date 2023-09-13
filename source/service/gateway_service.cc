
#include <unistd.h>

#include "gateway_service.h"
#include "service/base_service.h"
#include "utility/logger.h"

using namespace std;

GatewayService::GatewayService(const string& name) : BaseService(name) {}
// void ScriptService::ProcessMessages(int processing_num) {
//   for (int i = 0; i < processing_num; i++) {
//     bool succ = ProcessMessage();
//     if (!succ) {
//       break;
//     }
//   }
// }

void GatewayService::OnInitialization() { Info("Service {} 开始初始化", id_); }

void GatewayService::OnExit() { Info("Service {} 正在退出", id_); }

// bool ScriptService::ProcessMessage() {
//   auto message = message_queue_.PopFront();
//   if (message != nullptr) {
//     Info("服务 {} id:{} 处理消息", name_, id_);
//     if (message->type == BaseMessage::TYPE::SERVICE) {
//       auto service_message = dynamic_pointer_cast<ServiceMessage>(message);
//       Info("服务 {} id： {} 收到消息执行 {} 函数", name_, id_,
//            service_message->function_name_);
//       ExecuteJsFunction(service_message->function_name_);
//     }
//     // 新连接
//     else if (message->type == BaseMessage::TYPE::SOCKET_ACCEPT) {
//       auto msg = dynamic_pointer_cast<SocketAcceptMessage>(message);
//       cout << "OnAcceptMsg " << msg->client_fd_ << endl;
//       auto w = make_shared<ConnectionWriter>();
//       w->fd_ = msg->client_fd_;
//       writers.emplace(msg->client_fd_, w);
//     }
//     // SOCKET_RW
//     else if (message->type == BaseMessage::TYPE::SOCKET_RW) {
//       auto msg = dynamic_pointer_cast<SocketRWMessage>(message);
//       int fd = msg->fd_;
//       //可读
//       if (msg->is_read_) {
//         const int BUFFSIZE = 512;
//         char buff[BUFFSIZE];
//         int len = 0;
//         do {
//           len = read(fd, &buff, BUFFSIZE);
//           if (len > 0) {
//             OnSocketData(fd, buff, len);
//           }
//         } while (len == BUFFSIZE);

//         if (len <= 0 && errno != EAGAIN) {
//           if (XianNet::GetInstance().GetConnection(fd)) {
//             OnSocketClose(fd);
//             XianNet::GetInstance().CloseConnection(fd);
//           }
//         }
//       }
//       //可写（注意没有else）
//       if (msg->is_write_) {
//         if (XianNet::GetInstance().GetConnection(fd)) {
//           OnSocketWritable(fd);
//         }
//       }
//     }
//     return true;
//   }
//   return false;
// }

// // //收到客户端数据
// void ScriptService::OnSocketData(int fd, const char* buff, int len) {
//   cout << "OnSocketData" << fd << " buff: " << buff << endl;
//   //用ConnWriter发送大量数据
//   char* wirteBuff = new char[4200000];
//   wirteBuff[4200000 - 2] = 'e';
//   wirteBuff[4200000 - 1] = '\n';
//   int r = write(fd, wirteBuff, 4200000);
//   cout << "write r:" << r << " " << strerror(errno) << endl;
//   auto w = writers[fd];
//   w->EntireWrite(shared_ptr<char>(wirteBuff), 4200000);
//   w->LingerClose();
// }

// //套接字可写
// void ScriptService::OnSocketWritable(int fd) {
//   cout << "OnSocketWritable " << fd << endl;
//   auto w = writers[fd];
//   w->OnWriteable();
// }

// //关闭连接前
// void ScriptService::OnSocketClose(int fd) {
//   cout << "OnSocketClose " << fd << endl;
//   writers.erase(fd);
// }

void GatewayService::ProcessSocketAcceptMessage(SocketAcceptMessage* message) {
  Info("OnAcceptMsg client_fd: {}", message->client_fd_);
  // auto w = make_shared<ConnectionWriter>();
  // w->fd_ = msg->client_fd_;
  // writers.emplace(msg->client_fd_, w);
}

void GatewayService::ProcessSocketRwMessage(SocketRWMessage* message) {
  // TODO: 完善 服务对应的 UrlMapping
  if (message->can_read_) {
    string result = Read(message->fd_);
    Info("{} 服务, id: {} 接收到数据: {}", name_, id_, result);
    Info("{} 服务, id: {} 写入数据: {}", name_, id_, "hello world");
    Write(message->fd_, "hello world");
  }
  if (message->can_write_) {
    Info("{} 服务, id: {} 写入数据: {}", name_, id_, "hello world");
    Write(message->fd_, "hello world");
  }
}

std::string GatewayService::Read(int fd) {
  const int BUFF_SIZE = 64;
  char buff[BUFF_SIZE];
  int len = read(fd, &buff, BUFF_SIZE);
  string result = "";
  while (len > 0) {
    result += string(buff, len);
    len = read(fd, &buff, BUFF_SIZE);
  };
  // EAGAIN（数据读完）
  if (len == -1 && errno != EAGAIN) {
    // TODO:处理错误
  }
  return result;
}

void GatewayService::Write(int fd, const string& content) {
  write(fd, content.c_str(), content.length());
}
