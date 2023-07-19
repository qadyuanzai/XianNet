#pragma once

#include <fstream>
using namespace std;

class Logger {
public:
  // 日志等级
  enum class LEVEL { DEBUG, INFO, WARNING, ERROR, WELCOME };
private:
  ofstream output_file_stream_; // 将日志输出到文件的流对象
  LEVEL level_;                 // 日志等级
  string call_file_;
  string call_line_;
  string call_function_;
  bool is_write_to_log_file_ = false;

 private:
  Logger();
  void output(string text, LEVEL act_level); // 输出行为
  string GetCurrentTime();
  ~Logger();

public:
  static Logger &GetInstance() {
    static Logger instance;
    return instance;
  }
  Logger& SetFunctionCallInfo(string call_file, int call_line, string call_function);
  void Init(LEVEL level);
  void Init(LEVEL level, string log_file_path);
  void DEBUG(string text);
  void INFO(string text);
  void WARNING(string text);
  void ERROR(string text);

private:
  string greeting =
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

#define log Logger::GetInstance().SetFunctionCallInfo(__FILE__, __LINE__, __FUNCTION__)