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
class Config {
 public:
  static void LoadConfig(const string& config_path);
  static string GetString(const string& field_name);
  static int GetInt(const string& field_name);

 private:
  toml::table toml_table_;

 private:
  Config() {}
  static Config& GetInstance();
  template <typename T>
  static T GetValue(const string& field_name);
};

struct CoreConfig {
  static CoreConfig& GetInstance() {
    static CoreConfig instance;
    return instance;
  }
  string start_file_ = Config::GetString("core.start_file");
  string start_function_ = Config::GetString("core.start_function");
  string service_path_ = Config::GetString("core.service_path");
  int thread_size_ = Config::GetInt("core.thread_size");
};