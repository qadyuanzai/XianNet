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

 public:
  static Logger& GetInstance();
  void Init(LEVEL level);
  void Init(LEVEL level, string log_file_path);

  void Output(LEVEL act_level, const string& act_level_name,
              const experimental::source_location& location,
              const string& message);  // 输出行为

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

template <typename... Args>
struct Debug {
  Debug(format_string<Args...> format_message, Args&&... args,
        const std::experimental::source_location& location =
            std::experimental::source_location::current()) {
    Logger::GetInstance().Output(
        Logger::LEVEL::DEBUG, "DEBUG", location,
        vformat(format_message.get(), std::make_format_args(args...)));
  }
};
template <typename... Args>
Debug(format_string<Args...> format_message, Args&&...) -> Debug<Args...>;

template <typename... Args>
struct Info {
  Info(format_string<Args...> format_message, Args&&... args,
       const std::experimental::source_location& location =
           std::experimental::source_location::current()) {
    Logger::GetInstance().Output(
        Logger::LEVEL::INFO, "INFO", location,
        vformat(format_message.get(), std::make_format_args(args...)));
  }
};
template <typename... Args>
Info(format_string<Args...> format_message, Args&&...) -> Info<Args...>;

template <typename... Args>
struct Warning {
  Warning(format_string<Args...> format_message, Args&&... args,
          const std::experimental::source_location& location =
              std::experimental::source_location::current()) {
    Logger::GetInstance().Output(
        Logger::LEVEL::WARNING, "WARNING", location,
        vformat(format_message.get(), std::make_format_args(args...)));
  }
};
template <typename... Args>
Warning(format_string<Args...> format_message, Args&&...) -> Warning<Args...>;

template <typename... Args>
struct Error {
  Error(format_string<Args...> format_message, Args&&... args,
        const std::experimental::source_location& location =
            std::experimental::source_location::current()) {
    Logger::GetInstance().Output(
        Logger::LEVEL::ERROR, "ERROR", location,
        vformat(format_message.get(), std::make_format_args(args...)));
  }
};
template <typename... Args>
Error(format_string<Args...> format_message, Args&&...) -> Error<Args...>;