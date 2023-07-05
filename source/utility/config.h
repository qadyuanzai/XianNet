/*
 * @Author: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
 * @Date: 2023-05-23 17:59:09
 * @LastEditors: error: error: git config user.name & please set dead value or
 * install git && error: git config user.email & please set dead value or
 * install git & please set dead value or install git
 * @LastEditTime: 2023-05-24 16:09:44
 * @FilePath: /XianNet/include/config.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include <iostream>
#include <string>

#include "lib/toml/toml.hpp"

#define INT(x) int x = GetValue<int>(#x);
#define DOUBLE(x) double x = GetValue<double>(#x);
#define STRING(x) std::string x = GetValue<std::string>(#x);

using namespace std;
class Config {
 public:
  static Config& GetInstance() {
    static Config instance;
    return instance;
  }

 private:
  Config();
  ~Config(){};
  template <typename T>
  T GetValue(string field_name);

  toml::table config_ = toml::parse_file("config.toml");

 public:
  INT(thread_size_);
};
