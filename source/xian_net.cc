#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>

#include "lib/v8/include/libplatform/libplatform.h"
#include "message/message.h"
#include "service/base_service.h"
#include "service/gateway_service.h"
#include "service/script_service.h"
#include "utility/config.h"
#include "utility/logger.h"
#include "xian_net.h"

using namespace std;

XianNet::XianNet()
    : config_(Config::GetInstance()),
      platform_(v8::platform::NewDefaultPlatform()) {
  Info("XianNet 开始初始化");
  Info("thread_size: {}", config_.core.thread_size);
  // 忽略SIGPIPE信号
  signal(SIGPIPE, SIG_IGN);
  Logger::GetInstance().Initialize(Logger::LEVEL::DEBUG, "result.log");

  Info("v8引擎开始初始化");
  v8::V8::InitializePlatform(platform_.get());
  v8::V8::Initialize();
  create_params_.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();

  StartServiceWorker();
  StartSocketWorker();
}

XianNet::~XianNet() {
  // 关闭 v8 引擎
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  delete create_params_.array_buffer_allocator;
}

XianNet& XianNet::GetInstance() {
  static XianNet instance;
  return instance;
}

uint32_t XianNet::NewService(BaseService* service) {
  service_map_.AddService(service);
  service->OnInitialization();  // 初始化
  return service->id_;
}

uint32_t XianNet::NewScriptService(const string& name) {
  auto service = new ScriptService(create_params_, name);
  return NewService(service);
}

void XianNet::KillService(uint32_t id) {
  BaseService* service = GetService(id);
  if (!service) {
    return;
  }
  // 退出前
  service->OnExit();
  service->is_existing_ = true;
  // 删列表
  service_map_.Erase(id);
}

BaseService* XianNet::GetService(uint32_t id) { return service_map_.Find(id); }

void XianNet::BlockWorker() {
  mutex_lock_.Lock();
  sleep_count_++;
  thread_sleep_condition_.Wait(mutex_lock_.mutex());
  sleep_count_--;
  mutex_lock_.Unlock();
}

void XianNet::CheckAndWeakUpWorker() {
  /*
  这段代码还有一点需要注意，多个线程可能同时访问 sleepCount ，理论上需要加锁，但为了节省加锁的开销，此处直接读取 sleepCount 。 由于没加锁， sleepCount 的值可能不准确，尽管出现的概率较小，但不排除某次需要唤醒时没有唤醒（因为 sleepCount 的值小了）， 或不该唤醒时唤醒了（ sleepCount 的值大了）。但这两种错误无伤大雅，若出现“需要唤醒时没有唤醒”的情况，等待下一次唤醒即可 后续还会加上 Socket 模块，接收数据时也要唤醒工作线程）；“不该唤醒时唤醒了”更没关系，就当工作线程空转一次。综合起来， 我们认为错误唤醒的开销比给 sleepCount 加锁的开销小，于是宁愿偶尔出错也不加锁。
  */
  if (sleep_count_ == 0) {
    return;
  }
  if (config_.core.thread_size - sleep_count_ <=
      global_service_queue_.GetSize()) {
    Debug("唤起线程");
    thread_sleep_condition_.Signal();
  }
}

int XianNet::Listen(uint32_t port, uint32_t service_id) {
  //步骤1：创建socket
  /*
  int socket(int domain, int type, int protocol);
  参数:
    domain: 使用的地址族协议
      AF_INET: 使用IPv4格式的ip地址
      AF_INET6: 使用IPv4格式的ip地址
    type:
      SOCK_STREAM: 使用流式的传输协议
      SOCK_DGRAM: 使用报式(报文)的传输协议
    protocol: 一般写0即可, 使用默认的协议
      SOCK_STREAM: 流式传输默认使用的是tcp
      SOCK_DGRAM: 报式传输默认使用的udp

  返回值:
    成功: 可用于套接字通信的文件描述符
    失败: -1
  */
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd <= 0) {
    Error("listen error, listenFd <= 0");
    return -1;
  }
  //步骤2：设置为非阻塞
  /*
  O_NONBLOCK：防止为打开文件而阻塞很长时间。这通常仅对设备、网络、管道文件才有意义。此标志同时也作为I/O操作方式标志，这意味着在open中指明O_NONBLOCK就同时设置了非阻塞I/O方式。因此要非阻塞地打开一个文件且不影响正常的阻塞I/O，必须先设置O_NONBLOCK调用open，然后调用fcntl关闭此位。
  
  如果是阻塞，管道中没有数据，read会一直等待，直到有数据才会继续运行，否则一 直等待
  如果是非阻塞，read函数运行时，会先看一下管道中是否有数据，如果有数据，则正 常运行读取数据，如果管道中没有数据，则read函数会立即返回，继续下面的代码运行
  */
  fcntl(listen_fd, F_SETFL, O_NONBLOCK);
  //步骤3：bind 将文件描述符和本地的IP与端口进行绑定
  /*
  int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
  参数:
    sockfd: 监听的文件描述符, 通过socket()调用得到的返回值
    addr: 传入参数, 要绑定的IP和端口信息需要初始化到这个结构体中，IP和端口要转换为网络字节序
    addrlen: 参数addr指向的内存大小, sizeof(struct sockaddr)

  返回值：成功返回0，失败返回-1
  */
  struct sockaddr_in address;  //创建地址结构
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  int result = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
  if (result == -1) {
    Error("listen error, bind fail");
    return -1;
  }
  //步骤4：listen 给监听的套接字设置监听
  /*
  int listen(int sockfd, int backlog);
  参数:
    sockfd: 文件描述符, 可以通过调用socket()得到，在监听之前必须要绑定 bind()
    backlog: 同时能处理的最大连接要求，最大值为128

  返回值：函数调用成功返回0，调用失败返回 -1
  */
  result = listen(listen_fd, 64);
  if (result == -1) {
    Error("listen error, listen fail");
    return -1;
  }
  //步骤5：添加到连接集合
  AddConnection(listen_fd, service_id, Connection::TYPE::LISTEN);
  //步骤6：epoll事件，跨线程
  socket_worker_->AddEvent(listen_fd);
  return listen_fd;
}

void XianNet::CloseConnection(uint32_t socket_fd) {
  //删除conn对象
  bool succ = RemoveConnection(socket_fd);
  //关闭套接字
  close(socket_fd);
  //删除epoll对象对套接字的监听（跨线程）
  if (succ) {
    socket_worker_->RemoveEvent(socket_fd);
  }
}

void XianNet::SendMessage(uint32_t target_service_id, BaseMessage* message) {
  BaseService* target_service = GetService(target_service_id);
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
    CheckAndWeakUpWorker();
  }
}

/**
 * @brief 检查服务是否在全局消息队列中，若不在队列中则加入队列中
 * @param service 需要插入全局消息队列的服务
 * @return 若服务之前在全局消息队列中，则返回 true ，反之返回 false
 */
bool XianNet::CheckAndPushGlobalServiceQueue(BaseService* service) {
  /**
   * 书中做法是 设置服务 is_in_global 锁，后续读取 is_in_global
   * 都将被阻塞，在锁内完成 将服务插入“有消息待处理的服务”队列中 我的做法是 对
   * is_in_global 进行上锁、取值、赋值、解锁 服务插入全局队列不在锁内
   */
  bool is_in_global_queue_previous =
      service->is_in_global_queue_.SetCurrentAndGetPrevious(true);
  // 如果服务不在“有消息待处理的服务”队列中，则将该服务插入队列
  if (is_in_global_queue_previous == false) {
    Info("将 {} 服务加入全局服务队列中", service->name_);
    global_service_queue_.Push(service);
  }
  return !is_in_global_queue_previous;
}

void XianNet::StartServiceWorker() {
  for (int i = 0; i < config_.core.thread_size; i++) {
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
  // 创建线程
  socket_worker_thread_ = new thread(*socket_worker_);
}

int XianNet::AddConnection(int socket_fd, uint32_t service_id,
                           Connection::TYPE type) {
  auto connection = make_shared<Connection>();
  connection->fd_ = socket_fd;
  connection->service_id_ = service_id;
  connection->type_ = type;
  connection_map_.Emplace(socket_fd, connection);
  return socket_fd;
}

shared_ptr<Connection> XianNet::GetConnection(int socket_fd) {
  return connection_map_.Find(socket_fd);
}

bool XianNet::RemoveConnection(int socket_fd) {
  return connection_map_.Erase(socket_fd);
}

void XianNet::Wait() {
  cout << "XianNet OnWait" << endl;
  if (service_worker_thread_vector_[0]) {
    service_worker_thread_vector_[0]->join();
  }
}

BaseService* XianNet::PopGlobalServiceQueue() {
  auto service = global_service_queue_.PopFront();
  if (service != nullptr) {
    service->is_in_global_queue_.SetValue(false);
  }
  return service;
}

void XianNet::ModifyEvent(int socket_fd, bool epollOut) {
  socket_worker_->ModifyEvent(socket_fd, epollOut);
}

void XianNet::Init() {
  NewScriptService(config_.core.start_file);
  uint32_t gateway_service_id = NewService(new GatewayService("gateway"));
  Listen(config_.gateway.port, gateway_service_id);
};

/*
套接字-Socket https://subingwen.cn/linux/socket/?highlight=socket

*/
