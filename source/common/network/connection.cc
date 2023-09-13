#include <unistd.h>

#include "connection.h"

string Connection::ReadString() {
  const int BUFF_SIZE = 64;
  char buff[BUFF_SIZE];
  int len = read(fd_, &buff, BUFF_SIZE);
  string result = "";
  while (len > 0) {
    result += string(buff, len);
    len = read(fd_, &buff, BUFF_SIZE);
  };
  // EAGAIN（数据读完）
  if (len == -1 && errno != EAGAIN) {
    // TODO:处理错误
  }
  return result;
}

void Connection::WriteString(const string& content) {
  write(fd_, content.c_str(), content.length());
}
