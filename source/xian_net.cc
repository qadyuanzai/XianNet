#include "xian_net.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utility/logger.h"
#include <signal.h>

#include <iostream>
using namespace std;

XianNet::XianNet() : config_(Config::GetInstance()) {
  cout << "XianNet Start" << endl;
  // 忽略SIGPIPE信号
  signal(SIGPIPE, SIG_IGN);
  log.Init(Logger::LEVEL::DEBUG, "result.log");
  StartServiceWorker();
  StartSocketWorker();
}

XianNet& XianNet::GetInstance()  {
  static XianNet instance;
  return instance;
}

uint32_t XianNet::NewService(shared_ptr<string> type) {
  auto service = make_shared<Service>();
  service->type_ = type;
  service_map_.NewService(service);
  service->OnInit();  // 初始化
  return service->id_;
}

void XianNet::KillService(uint32_t id) {
  shared_ptr<Service> service = GetService(id);
  if (!service) {
    return;
  }
  // 退出前
  service->OnExit();
  service->is_existing_ = true;
  // 删列表
  service_map_.Erase(id);
}

shared_ptr<Service> XianNet::GetService(uint32_t id)  {
  return service_map_.Find(id);
}

void XianNet::BlockWorker() { 
  mutex_lock_.Lock(); 
  sleep_count_++;
  thread_sleep_condition_.Wait(mutex_lock_.mutex());
  sleep_count_--;
  mutex_lock_.Unlock();
}

void XianNet::CheckAndWeakUpWorker() {
  /*
  这段代码还有一点需要注意，多个线程可能同时访问 sleepCount ，理论上需要加锁，但为了节省加锁的开销，此处直接读取 sleepCount 。
  由于没加锁， sleepCount 的值可能不准确，尽管出现的概率较小，但不排除某次需要唤醒时没有唤醒（因为 sleepCount 的值小了），
  或不该唤醒时唤醒了（ sleepCount 的值大了）。但这两种错误无伤大雅，若出现“需要唤醒时没有唤醒”的情况，等待下一次唤醒即可
  （后续还会加上 Socket 模块，接收数据时也要唤醒工作线程）；“不该唤醒时唤醒了”更没关系，就当工作线程空转一次。综合起来，
  我们认为错误唤醒的开销比给 sleepCount 加锁的开销小，于是宁愿偶尔出错也不加锁。
  */
  if(sleep_count_ == 0) {
      return;
  }
  if(config_.thread_size_ - sleep_count_ <= global_service_queue_.GetSize()) {
      cout << "weakup" << endl;
      thread_sleep_condition_.Signal();
  }
}

int XianNet::Listen(uint32_t port, uint32_t serviceId) { 
    //步骤1：创建socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd <= 0){
        cout << "listen error, listenFd <= 0" << endl;
        return -1;
    }
    //步骤2：设置为非阻塞
    fcntl(listenFd, F_SETFL, O_NONBLOCK);
    //步骤3：bind
    struct sockaddr_in addr;   //创建地址结构
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int r = bind(listenFd, (struct sockaddr*)&addr, sizeof(addr));
    if( r == -1){
        cout << "listen error, bind fail" << endl;
        return -1;
    }
    //步骤4：listen
    r = listen(listenFd, 64); 
        if(r < 0){
        return -1;
    }
    //步骤5：添加到管理结构
    AddConnection(listenFd, serviceId, Connection::TYPE::LISTEN);
    //步骤6：epoll事件，跨线程
    socket_worker_->AddEvent(listenFd);
    return listenFd;

}

void XianNet::CloseConn(uint32_t fd) {
      //删除conn对象
    bool succ = RemoveConnection(fd);
    //关闭套接字
    close(fd);
    //删除epoll对象对套接字的监听（跨线程）
    if(succ) {
        socket_worker_->RemoveEvent(fd);
    }
}

void XianNet::SendMessage(uint32_t target_service_id,
                          shared_ptr<BaseMessage> message) {
  shared_ptr<Service> target_service = GetService(target_service_id);
  if (!target_service) {
    cout << "Send fail, toSrv not exist toId:" << target_service_id << endl;
    return;
  }
  // 将消息插入目标服务内的消息队列
  target_service->PushMessage(message);
  // 检查并放入全局队列
  bool has_push = CheckAndPushGlobalServiceQueue(target_service);
  // 唤起进程
  if (has_push) {
    cout << "唤起线程" << endl;
    CheckAndWeakUpWorker();
  }
}

shared_ptr<BaseMessage> XianNet::MakeMsg(uint32_t source, char* buff, int len) {
  auto msg= make_shared<ServiceMessage>();
    msg->type = BaseMessage::TYPE::SERVICE;
    msg->source = source;
    //基本类型的对象没有析构函数，
    //所以用delete 或 delete[]都可以销毁基本类型数组；
    //智能指针默认使用delete销毁对象，
    //所以无须重写智能指针的销毁方法
    msg->buff = shared_ptr<char>(buff);
    msg->size = len;
    return msg;
}

/**
 * @brief 检查服务是否在全局消息队列中，若不在队列中则加入队列中
 * @param service 需要插入全局消息队列的服务
 * @return 若服务之前在全局消息队列中，则返回 true ，反之返回 false
 */
bool XianNet::CheckAndPushGlobalServiceQueue(shared_ptr<Service> service)  {
  /**
   * 书中做法是 设置服务 is_in_global 锁，后续读取 is_in_global 都将被阻塞，在锁内完成 将服务插入“有消息待处理的服务”队列中
   * 我的做法是 对 is_in_global 进行上锁、取值、赋值、解锁 服务插入全局队列不在锁内
  */
  bool is_in_global_queue_previous = service->is_in_global_queue_.SetCurrentAndGetPrevious(true);
  // 如果服务不在“有消息待处理的服务”队列中，则将该服务插入队列
  if (is_in_global_queue_previous == false) {
    cout << "push to global queue" << endl;
    global_service_queue_.Push(service);
  }
  return !is_in_global_queue_previous;
}

void XianNet::StartServiceWorker() {
  for (int i = 0; i < config_.thread_size_; i++) {
    ServiceWorker* service_worker = new ServiceWorker();
    service_worker->id_ = i;
    service_worker->processing_num_ = 1 << i;
    // 创建线程
    thread* service_worker_thread = new thread(*service_worker);
    // 添加到数组
    service_worker_vector_.push_back(service_worker);
    service_worker_thread_vector_.push_back(service_worker_thread);
  }
}

void XianNet::StartSocketWorker() {
  // 创建线程对象
  socket_worker_ = new SocketWorker();
  // 初始化
  socket_worker_->Init();
  // 创建线程
  socket_worker_thread_ = new thread(*socket_worker_);
}

int XianNet::AddConnection(int fd, uint32_t id, Connection::TYPE type) {
  auto connection = make_shared<Connection>();
  connection->fd_ = fd;
  connection->service_id_ = id;
  connection->type_ = type;
  connection_map_.Emplace(fd, connection);
  return fd;
}

shared_ptr<Connection> XianNet::GetConnection(int fd) {
  return connection_map_.Find(fd);
}

bool XianNet::RemoveConnection(int fd) { return connection_map_.Erase(fd); }

void XianNet::Wait()  {
  cout << "XianNet OnWait" << endl;
  if (service_worker_thread_vector_[0]) {
    service_worker_thread_vector_[0]->join();
  }
}

shared_ptr<Service> XianNet::PopGlobalServiceQueue() {
  auto service = global_service_queue_.PopFront();
  if (service != nullptr) {
    service->is_in_global_queue_.SetValue(false);
  }
  return service;
}
void XianNet::ModifyEvent(int fd, bool epollOut) {
    socket_worker_->ModifyEvent(fd, epollOut);
}