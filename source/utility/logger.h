#pragma once

#include <format>
#include <fstream>
using namespace std;

class Logger {
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
  static Logger& GetInstance() {
    static Logger instance;
    return instance;
  }
  void Init(LEVEL level);
  void Init(LEVEL level, string log_file_path);

  void Output(string call_file, int call_line, string call_function,
              LEVEL act_level, string message);  // 输出行为

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
#define log Logger::GetInstance()
#define LOGINFO(act_level, format_message, ...)       \
  Output(__FILE__, __LINE__, __FUNCTION__, act_level, \
         format(format_message __VA_OPT__(, ) __VA_ARGS__))

#define DEBUG(format_message, ...) \
  LOGINFO(Logger::LEVEL::DEBUG, format_message, __VA_ARGS__)

#define INFO(format_message, ...) \
  LOGINFO(Logger::LEVEL::INFO, format_message, __VA_ARGS__)

#define WARNING(format_message, ...) \
  LOGINFO(Logger::LEVEL::WARNING, format_message, __VA_ARGS__)

#define ERROR(format_message, ...) \
  LOGINFO(Logger::LEVEL::ERROR, format_message, __VA_ARGS__)
