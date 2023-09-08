/**
 * @file logger.h
 * @author zsy (974483053@qq.com)
 * @brief 日志模块
 * @version 0.1
 * @date 2023-09-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <experimental/source_location>
#include <format>
#include <fstream>

#include "common/lock/spin_lock.h"
using namespace std;

class Logger : public SpinLock {
 public:
  // 日志等级
  enum class LEVEL { DEBUG, INFO, WARNING, ERROR };

 private:
  ofstream output_file_stream_;  // 将日志输出到文件的流对象
  LEVEL level_;                  // 日志等级
  bool is_write_to_log_file_ = false;

 private:
  ~Logger();
  string GetCurrentFormattedTimeString();
  void Output(const string& output_content);

 public:
  static Logger& GetInstance();
  void Initialize(LEVEL level);
  void Initialize(LEVEL level, string log_file_path);

  void Log(LEVEL act_level, const string& act_level_name, const string& message,
           const experimental::source_location& location);
  void JsLog(LEVEL act_level, const string& act_level_name,
             const string& message, const string& file_name);

 private:
  string greeting_ =
      "///////////////////////////////////////////////////////////////\n"
      "//                          _ooOoo_                          //\n"
      "//                         o8888888o                         //\n"
      "//                         88\" . \"88                         //\n"
      "//                         (| ^_^ |)                         //\n"
      "//                         O\\  =  /O                         //\n"
      "//                      ____/`---'\\____                      //\n"
      "//                    .'  \\\\|     |//  `.                    //\n"
      "//                   /  \\\\|||  :  |||//  \\                   //\n"
      "//                  /  _||||| -:- |||||-  \\                  //\n"
      "//                  |   | \\\\\\  -  /// |   |                  //\n"
      "//                  | \\_|  ''\\---/''  |   |                  //\n"
      "//                  \\  .-\\__  `-`  ___/-. /                  //\n"
      "//                ___`. .'  /--.--\\  `. . ___                //\n"
      "//              .\"\"'<  `.___\\_<|>_/___.'  >'\"\".              //\n"
      "//            | | : `- \\\\`.;`\\ _ /`;.`/ - ` : | |            //\n"
      "//            \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /            //\n"
      "//      =======`-.____`-.___\\_____/___.-`____.-'=======      //\n"
      "//                          `=---='                          //\n"
      "//      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^      //\n"
      "//             佛祖保佑     永不宕机     永无BUG             //\n"
      "///////////////////////////////////////////////////////////////\n";
};

#define DECLARE_LOG(log_function_name, log_level)                            \
  template <typename... Args>                                                \
  struct log_function_name {                                                 \
    log_function_name(format_string<Args...> format_message, Args&&... args, \
                      const experimental::source_location& location =        \
                          experimental::source_location::current()) {        \
      Logger::GetInstance().Log(                                             \
          Logger::LEVEL::log_level, #log_level,                              \
          vformat(format_message.get(), std::make_format_args(args...)),     \
          location);                                                         \
    }                                                                        \
  };                                                                         \
  template <typename... Args>                                                \
  log_function_name(format_string<Args...> format_message, Args&&...)        \
      -> log_function_name<Args...>;                                         \
  struct Js##log_function_name {                                             \
    Js##log_function_name(string message, string file_name) {                \
      Logger::GetInstance().JsLog(Logger::LEVEL::log_level, #log_level,      \
                                  message, file_name);                       \
    }                                                                        \
  };

DECLARE_LOG(Debug, DEBUG);
DECLARE_LOG(Info, INFO);
DECLARE_LOG(Warning, WARNING);
DECLARE_LOG(Error, ERROR);

#undef DECLARE_LOG