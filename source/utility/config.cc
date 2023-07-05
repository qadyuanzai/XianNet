#include "config.h"

#include <iostream>
#include <string>

Config::Config() {}

template <typename T>
T Config::GetValue(string field_name) {
  if (field_name[field_name.size() - 1] == '_') {
    field_name = field_name.erase(field_name.size() - 1, 1);
  }
  return config_[field_name].value<T>().value();
}
