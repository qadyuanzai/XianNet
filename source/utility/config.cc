#include "config.h"

TomlConfig& TomlConfig::GetInstance() {
  static TomlConfig instance;
  return instance;
}

void TomlConfig::LoadConfig(const string& config_path) {
  GetInstance().toml_table_ = toml::parse_file(config_path);
}

string TomlConfig::GetString(const string& field_name) {
  return GetValue<string>(field_name);
}

int TomlConfig::GetInt(const string& field_name) {
  return GetValue<int>(field_name);
}

template <typename T>
T TomlConfig::GetValue(const string& field_name) {
  return GetInstance().toml_table_.at_path(field_name).value<T>().value();
}
