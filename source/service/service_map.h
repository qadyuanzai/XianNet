#pragma once

#include "common/thread_safe_container/rwlock_unordered_map.h"
#include "service/script_service.h"
using namespace std;
class ServiceMap : public RwlockUnorderedMap<uint32_t, BaseService*> {
 private:
  uint32_t auto_increment_id_ = 0;

 public:
  void AddService(BaseService* service) {
    LockWrite();
    service->id_ = auto_increment_id_;
    map_.emplace(service->id_, service);
    auto_increment_id_++;
    Unlock();
  }
};