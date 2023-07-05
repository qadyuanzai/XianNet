#pragma once

#include "helper/thread_safe_container/rwlock_unordered_map.h"
#include "service/service.h"
using namespace std;
class ServiceMap
    : public RwlockUnorderedMap<uint32_t, shared_ptr<Service>> {
 private:
  uint32_t auto_increment_id_ = 0;
 public:
  void NewService(shared_ptr<Service> service) {
    LockWrite();
    service->id_ = auto_increment_id_;
    map_.emplace(service->id_, service);
    auto_increment_id_++;
    Unlock();
  }
};