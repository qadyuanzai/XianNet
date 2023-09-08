#include "service_worker.h"

#include <unistd.h>

#include <iostream>

#include "utility/logger.h"
#include "xian_net.h"
using namespace std;

void ServiceWorker::CheckAndPutGlobal(shared_ptr<Service> service) {
  if (service->is_existing_) {
    return;
  }
  // 如果服务中还有消息则将该服务重新插入全局消息队列中
  if (!service->IsMessageQueueEmpty()) {
    XianNet::GetInstance().CheckAndPushGlobalServiceQueue(service);
  }
}

void ServiceWorker::operator()() {
  while (true) {
    auto service = XianNet::GetInstance().PopGlobalServiceQueue();
    if (service == nullptr) {
      XianNet::GetInstance().BlockWorker();
      Debug("线程id: {} 阻塞", id_);
    } else {
      Debug("线程id: {} 处理 {} 服务, 处理消息数量: {}", id_, service->name_,
            processing_num_);
      service->ProcessMessages(processing_num_);
      CheckAndPutGlobal(service);
    }
  }
}