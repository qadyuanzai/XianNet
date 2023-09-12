#include <unistd.h>

#include "service/gateway_service.h"
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
  TomlConfig::LoadConfig(parant_path + argv[1]);

  XianNet::GetInstance().Init();

  // uint32_t gateway_id = XianNet::GetInstance().NewService();
  // int fd = XianNet::GetInstance().Listen(8001, gateway_id);
  //   usleep(15 * 1000000);
  //   XianNet::GetInstance().CloseConn(fd);
  //等待
  Info("\n(╯°□°）╯︵ ┻━┻   XianNet启动成功   ┳━┳ノ(°-°ノ)\n");
  XianNet::GetInstance().Wait();
  return 0;
}