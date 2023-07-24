#include "logger.h"

#include <iostream>
#include <string>

#include "lib/magic_enum/magic_enum.hpp"
#include "lib/opentime/opentime.h"

void Logger::Init(LEVEL level) {
  level_ = level;
  cout << greeting_ << endl;
}

void Logger::Init(LEVEL level, string log_file_path) {
  is_write_to_log_file_ = true;
  output_file_stream_.open(log_file_path, ios::out | ios::app);  // 打开输出文件
  Init(level);
}

Logger::~Logger() { output_file_stream_.close(); }

void Logger::Output(string call_file, int call_line, string call_function,
                    LEVEL act_level, string message) {
  OpenTime open_time;
  string output_content = open_time.toString() + " " + call_file + ":" +
                          to_string(call_line) + " " + call_function + " [" +
                          string(magic_enum::enum_name(act_level)) + "] - " +
                          message + "\n";
  if (this->level_ <= act_level) {
    // 当前等级设定的等级才会显示在终端，且不能是只文件模式
    cout << output_content;
  }
  if (is_write_to_log_file_) {
    output_file_stream_ << output_content;
    output_file_stream_.flush();
  }
}
