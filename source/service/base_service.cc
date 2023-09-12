#include "base_service.h"
#include "utility/logger.h"

void BaseService::ProcessMessages(int processing_num) {
  for (int i = 0; i < processing_num; i++) {
    auto message = message_queue_.PopFront();
    if (message != nullptr) {
      if (message->type_ == BaseMessage::TYPE::SERVICE) {
        Info("处理 SERVICE 消息");
        ProcessServiceMessage((ServiceMessage*)message);
      } else if (message->type_ == BaseMessage::TYPE::SOCKET_ACCEPT) {
        Info("处理 SOCKET_ACCEPT 消息");
        ProcessSocketAcceptMessage((SocketAcceptMessage*)message);
      } else if (message->type_ == BaseMessage::TYPE::SOCKET_RW) {
        Info("处理 SOCKET_RW 消息");
        ProcessSocketRwMessage((SocketRWMessage*)message);
      }
    }
  }
}
void BaseService::PushMessage(BaseMessage* message) {
  message_queue_.Push(message);
}
bool BaseService::IsMessageQueueEmpty() { return message_queue_.IsEmpty(); }
BaseService::BaseService(const string& name) : name_(name) {}
