#include "logger.h"
#include "lib/magic_enum/magic_enum.hpp"
#include "lib/opentime/opentime.h"
#include <string>
#include <iostream>
#include <stdio.h>

Logger::Logger() {
  cout << greeting << endl;
};

Logger& Logger::SetFunctionCallInfo(string call_file, int call_line, string call_function) {
  call_file_ = call_file;
  call_line_ = to_string(call_line);
  call_function_ = call_function;
  return *this;
}

void Logger::Init(LEVEL level) {
  level_ = level;
  output("\n=============== Start logging ===============", LEVEL::WELCOME);
}

void Logger::Init(LEVEL level, string log_file_path) {
  is_write_to_log_file_ = true;
  output_file_stream_.open(log_file_path, ios::out | ios::app); // 打开输出文件
  Init(level);
}

Logger::~Logger() { output_file_stream_.close(); }

void Logger::output(string text, LEVEL act_level) {
  string output_content = GetCurrentTime() + " " + call_file_ + ":" +
                          call_line_ + " " + call_function_ + " [" +
                          string(magic_enum::enum_name(act_level)) + "] - " +
                          text + "\n";
  if (this->level_ <= act_level) {
    // 当前等级设定的等级才会显示在终端，且不能是只文件模式
    cout << output_content;
  }
  if (is_write_to_log_file_) {
    output_file_stream_ << output_content;
    output_file_stream_.flush();
  }
}

string Logger::GetCurrentTime() {
  OpenTime open_time;
  return open_time.toString();
}

void Logger::DEBUG(string text) { this->output(text, LEVEL::DEBUG); }

void Logger::INFO(string text) { this->output(text, LEVEL::INFO); }

void Logger::WARNING(string text) { this->output(text, LEVEL::WARNING); }

void Logger::ERROR(string text) { this->output(text, LEVEL::ERROR); }

