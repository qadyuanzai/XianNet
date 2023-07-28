#include "config.h"

Config& Config::GetInstance() {
  static Config instance;
  return instance;
}

void Config::LoadConfig(const string& config_path) {
  GetInstance().toml_table_ = toml::parse_file(config_path);
}

string Config::GetString(const string& field_name) {
  return GetValue<string>(field_name);
}

int Config::GetInt(const string& field_name) {
  return GetValue<int>(field_name);
}

template <typename T>
T Config::GetValue(const string& field_name) {
  return GetInstance().toml_table_.at_path(field_name).value<T>().value();
}
