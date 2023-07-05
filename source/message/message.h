/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-05-23 17:59:09
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-05-25 16:19:27
 * @FilePath: /XianNet/include/message.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include <memory>
using namespace std;

class BaseMessage {
public:
	enum TYPE
	{
		SERVICE,
		SOCKET_ACCEPT,
		SOCKET_RW,
	};
	TYPE type;
	char load[1000000]{};
	virtual ~BaseMessage() {};
};

class ServiceMessage : public BaseMessage {
public:
	uint32_t source;		//消息发送方
	shared_ptr<char> buff;	//消息内容
	size_t size;			//消息内容大小
};

//有新连接
class SocketAcceptMessage : public BaseMessage {
public: 
	int listen_fd_;
	int client_fd_;
};

//可读可写
class SocketRWMessage : public BaseMessage {
public: 
	int fd_;
	bool is_read_ = false;
	bool is_write_ = false;
};