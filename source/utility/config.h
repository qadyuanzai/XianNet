/*
 * @file config.h
 * @author 钝角 (974483053@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-07-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "lib/toml/toml.hpp"

using namespace std;
class TomlConfig {
 public:
  static void LoadConfig(const string& config_path);
  static string GetString(const string& field_name);
  static int GetInt(const string& field_name);

 private:
  toml::table toml_table_;

 private:
  TomlConfig() {}
  static TomlConfig& GetInstance();
  template <typename T>
  static T GetValue(const string& field_name);
};

struct Config {
  static Config& GetInstance() {
    static Config instance;
    return instance;
  }
  struct Core {
    string start_file = TomlConfig::GetString("core.start_file");
    string start_function = TomlConfig::GetString("core.start_function");
    string service_path = TomlConfig::GetString("core.service_path");
    int thread_size = TomlConfig::GetInt("core.thread_size");
  } core;
  struct Gateway {
    int port = TomlConfig::GetInt("gateway.port");
  } gateway;
};
