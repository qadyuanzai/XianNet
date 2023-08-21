#include <unistd.h>

#include "utility/config.h"
#include "utility/logger.h"
#include "xian_net.h"

// #include "xian_net.h"

// void TestSocketCtrl() {
//   int fd = XianNet::GetInstance().Listen(8001, 1);
//   usleep(15 * 1000000);
//   XianNet::GetInstance().CloseConn(fd);
// }

// void TestEcho() {
//   uint32_t gateway = XianNet::GetInstance().NewService("gateway");
// }

int main(int argc, char* argv[]) {
  string parant_path = argv[0];
  parant_path = parant_path.substr(0, parant_path.find_last_of('/') + 1);
  Config::LoadConfig(parant_path + argv[1]);

  XianNet::GetInstance().Init();
  //等待
  Info("\n(╯°□°）╯︵ ┻━┻   XianNet启动成功   ┳━┳ノ(°-°ノ)");
  XianNet::GetInstance().Wait();
  return 0;
}