#include"Socket.h"
#include<cstdio>
#include<utility>

//默认构造函数：出生即创建底层套接字
Socket::Socket() :
	m_fd(::socket(AF_INET,SOCK_STREAM,0)){
		assert(m_fd >= 0);
	}

//显式构造函数：用已有fd接管资源（比如accept产生的clientFd）
Socket::Socket(int fd) : m_fd(fd) {}

//析构函数：自动安全释放资源（RAII）
Socket::~Socket() {
	close();
}

//移动构造函数：转移句柄所有权，将原对象的m_fd置为-1
Socket::Socket(Socket&& other) noexcept : m_fd(other.m_fd) {
	other.m_fd = -1;
}

//移动赋值运算符：先释放自身资源，再接管对方句柄
Socket& Socket::operator=(Socket&& other) noexcept {
	if(this != &other) {
		close(); //先关闭自己原有的fd
		m_fd = other.m_fd; //接管对方的fd
		other.m_fd = -1; //剥夺对方控制权，防止重复close
	}
	return *this;
}

//绑定端口（包含setsockopt返回值检查）
bool Socket::bind(uint16_t port) {
	if(m_fd < 0) return false;

	int reuse = 1;
	if(::setsockopt(m_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse)) < 0) {
		return false;
	}

	struct sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	return 0 == ::bind(m_fd,(struct sockaddr*)&addr,sizeof(addr));
}

//开启监听
bool Socket::listen(int backlog) {
	if(m_fd < 0) return false;
	return 0 == ::listen(m_fd,backlog);
}

//接收客户端连接
int Socket::accept() {
	struct sockaddr_in addr{};
	socklen_t len = sizeof(addr);
	return ::accept(m_fd,(struct sockaddr*)&addr,&len);
}

//设置非阻塞模式（包含fcntl返回值检查）
bool Socket::setNonBlocking(){
	if(m_fd < 0) return false;
	int oldOption = ::fcntl(m_fd,F_GETFL);
	if(oldOption < 0) return false;
	int newOption = oldOption | O_NONBLOCK;
	return 0 == ::fcntl(m_fd,F_SETFL,newOption);
}

//安全关闭套接字（防止重复close）
void Socket::close() {
	if(m_fd >= 0) {
		::close(m_fd);
		m_fd = -1;
	}
}

//Getter:获取底层fd
int Socket::fd() const {
	return m_fd;
}


//Getter: 检查fd是否有效
bool Socket::valid() const {
	return m_fd >= 0;
}
