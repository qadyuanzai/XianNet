#include <chrono>
#include <iostream>

#include "lib/opentime/opentime.h"
#include "logger.h"

void Logger::Initialize(LEVEL level) {
  level_ = level;
  cout << greeting_ << endl;
}

void Logger::Initialize(LEVEL level, string log_file_path) {
  is_write_to_log_file_ = true;
  output_file_stream_.open(log_file_path, ios::out | ios::app);  // 打开输出文件
  Initialize(level);
}

string Logger::GetCurrentFormattedTimeString() {
  OpenTime open_time;
  long ms = duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count() %
            1000;
  return open_time.toString() + "." + to_string(ms) + " ";
}

void Logger::Output(const string& output_content) {
  Lock();
  cout << output_content;
  if (is_write_to_log_file_) {
    output_file_stream_ << output_content;
  }
  Unlock();
}

void Logger::Log(LEVEL act_level, const string& act_level_name,
                 const string& message,
                 const experimental::source_location& location) {
  // 满足日志记录等级要求才会输出
  if (this->level_ <= act_level) {
    const string& file_name = location.file_name();
    string location_string = file_name.substr(file_name.find('/') + 1) + ":" +
                             to_string(location.line()) + " " +
                             location.function_name();
    string output_content = GetCurrentFormattedTimeString() + location_string +
                            " [" + act_level_name + "] - " + message + "\n";
    Output(output_content);
  }
}

void Logger::JsLog(LEVEL act_level, const string& act_level_name,
                   const string& message, const string& file_name) {
  // 满足日志记录等级要求才会输出
  if (this->level_ <= act_level) {
    string output_content = GetCurrentFormattedTimeString() + "[SCRIPT] " +
                            file_name + " [" + act_level_name + "] - " +
                            message + "\n";
    Output(output_content);
  }
}

Logger::~Logger() { output_file_stream_.close(); }
Logger& Logger::GetInstance() {
  static Logger instance;
  return instance;
}
