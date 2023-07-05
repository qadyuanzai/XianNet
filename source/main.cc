#include "xian_net.h"

void test() {
  auto pingType = make_shared<string>("ping");
  uint32_t ping1 = XianNet::GetInstance().NewService(pingType);
  uint32_t ping2 = XianNet::GetInstance().NewService(pingType);
  uint32_t pong = XianNet::GetInstance().NewService(pingType);
  auto msg1 = XianNet::GetInstance().MakeMsg(ping1, new char[3] { 'h', 'i', '\0' }, 3);
  // auto msg2 = XianNet::GetInstance().MakeMsg(ping2, new char[6] { 'h', 'e', 'l', 'l', 'o', '\0' }, 6);
  XianNet::GetInstance().SendMessage(pong, msg1);
}

int main() {
  test();
  XianNet::GetInstance().Wait();
  return 0;
}