#include "ConnWriter.h"
#include <unistd.h>
#include "xian_net.h"
#include <iostream>
#include <string.h>
#include <sys/socket.h>

void ConnWriter::EntireWriteWhenEmpty(shared_ptr<char> buff, streamsize len) {
    char* s = buff.get() ;
    //谨记：>=0, -1&&EAGAIN, -1&&EINTR, -1&&其他
    streamsize n = write(fd, s, len);
    if(n < 0 && errno == EINTR) { }; //仅提醒你要注意
    cout << "EntireWriteWhenEmpty write n=" << n << endl;
    //情况1-1：全部写完
    if(n >= 0 && n == len) {
        return;
    }
    //情况1-2：写一部分（或没写入）
    if( (n > 0 && n < len) || (n < 0 && errno == EAGAIN) ) {
        auto obj = make_shared<WriteObject>();
        obj->start = n;
        obj->buff = buff;
        obj->len = len;
        objs.push_back(obj);
        XianNet::GetInstance().ModifyEvent(fd, true);
        return;
    }
    //情况1-3：真的发生错误
    cout << "EntireWrite write error " <<  endl;
}

void ConnWriter::EntireWriteWhenNotEmpty(shared_ptr<char> buff, streamsize len) {
    auto obj = make_shared<WriteObject>();
    obj->start = 0;
    obj->buff = buff;
    obj->len = len;
    objs.push_back(obj);
}

void ConnWriter::EntireWrite(shared_ptr<char> buff, streamsize len) {
    if(isClosing){
        cout << "EntireWrite fail, because isClosing" << endl;
        return;
    }
    //情况1：没有待写入数据，先尝试写入
    if(objs.empty()) {
        EntireWriteWhenEmpty(buff, len);
    }
    //情况2：有待写入数据，添加到末尾
    else{
        EntireWriteWhenNotEmpty(buff, len);
    }
}


//返回值:是否完整的写入了一条
bool ConnWriter::WriteFrontObj() {
    //没待写数据
    if(objs.empty()) {
        return false;
    }
    //获取第一条
    auto obj = objs.front();

    //谨记：>=0, -1&&EAGAIN, -1&&EINTR, -1&&其他
    char* s = obj->buff.get() + obj->start;
    int len = obj->len - obj->start;
    int n = write(fd, s, len);
    cout << "WriteFrontObj write n=" << n << endl;
    if(n < 0 && errno == EINTR) { }; //仅提醒你要注意
    //情况1-1：全部写完
    if(n >= 0 && n == len) {
        objs.pop_front(); //出队
        return true;
    }
    //情况1-2：写一部分（或没写入）
    if( (n > 0 && n < len) || (n < 0 && errno == EAGAIN) ) {
        obj->start += n;
        return false;
    }
    //情况1-3：真的发生错误
    cout << "EntireWrite write error " << endl;
}

void ConnWriter::OnWriteable() {
    auto conn = XianNet::GetInstance().GetConnection(fd);
    if(conn == NULL){ //连接已关闭
        return;
    }

    while(WriteFrontObj()){
        //循环
    }
    
    if(objs.empty()) {
        XianNet::GetInstance().ModifyEvent(fd, false);

        if(isClosing) {
            //通知服务，此处并不是通用做法
            //让read产生 Bad file descriptor报错
            cout << "linger close conn" << endl;
            shutdown(fd, SHUT_RD);
            auto msg= make_shared<SocketRWMessage>();
            msg->type = BaseMessage::TYPE::SOCKET_RW;
            msg->fd_ = conn->fd_;
            msg->is_read_ = true;
            XianNet::GetInstance().SendMessage(conn->service_id_, msg);
        }
    }
}

void ConnWriter::LingerClose(){
    if(isClosing){
        return;
    }
    isClosing = true;
    if(objs.empty()) {
        XianNet::GetInstance().CloseConn(fd);
        return;
    }
}
