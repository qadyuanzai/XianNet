#pragma once
#include <stdint.h>

class Connection {
 public:
  enum class TYPE {          //消息类型
      LISTEN = 1, 
      CLIENT = 2,
  }; 
  
  TYPE type_;
  int fd_;
  uint32_t service_id_;
};