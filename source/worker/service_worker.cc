#include "service_worker.h"

#include <iostream>
#include <unistd.h>

#include "xian_net.h"
using namespace std;

void ServiceWorker::CheckAndPutGlobal(shared_ptr<Service> service) {
  if(service->is_existing_){ 
      return; 
  }
  // 如果服务中还有消息则将该服务重新插入全局消息队列中
  if(!service->IsMessageQueueEmpty()){
    XianNet::GetInstance().CheckAndPushGlobalServiceQueue(service);
  }
}

void ServiceWorker::operator()() {
  while (true) {
    auto service = XianNet::GetInstance().PopGlobalServiceQueue();
    if (service == nullptr) {
      XianNet::GetInstance().BlockWorker();
      cout << "线程id:" << id_ << " 阻塞" << endl;
    } else {
      cout << "working id:" << id_ << ", processing_num:" << processing_num_ << endl;
      service->ProcessMessages(processing_num_);
      CheckAndPutGlobal(service);
    }
  }
}