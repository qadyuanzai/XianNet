/**
 * @file connection.h
 * @author zsy (974483053@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <stdint.h>

#include <string>
using namespace std;

class Connection {
 public:
  enum class TYPE {  //消息类型
    LISTEN,
    CLIENT
  };

  TYPE type_;
  int fd_;
  uint32_t service_id_;

  string ReadString();
  void WriteString(const string& content);
};