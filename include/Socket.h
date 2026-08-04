#ifndef SOCKET_H
#define SOCKET_H

#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<cstdint>
#include<cassert>

class Socket {
public:
	//默认构造函数：对象出生即创建底层socket_fd
	Socket();

	//显式构造函数：用已有fd包装Socket对象（供accept内部或特定场景使用）
	explicit Socket(int fd);

	//析构函数：自动调用close()释放资源（RAII）
	~Socket();

	//禁止拷贝构造函数与拷贝赋值（独占句柄资源）
	Socket(const Socket&) = delete;
	Socket& operator=(const Socket&) = delete;

	//允许移动构造与移动赋值（转移句柄所有权，支持accept返回对象）
	Socket(Socket&& other) noexcept;
	Socket& operator=(Socket&& other) noexcept;
        
	//服务端步骤：设置端口复用（SO_REUSEADDR）并绑定端口（默认INADDR_ANY）
	bool bind(uint16_t port);

	//服务器步骤：开启监听(默认SOMAXCONN)
	bool listen(int backlog = SOMAXCONN);

	//服务器步骤：接收客户端连接，直接返回封装好的Socket对象（全链路RAII）
	int accept();

	//辅助功能：设置非阻塞模式（配合epoll Et 边沿触发）
	bool setNonBlocking();

	//辅助功能：安全关闭套接字（将m_fd置为-1,防止重复close）
	void close();

	//Getter:获取底层套接字文件描述符（供Epoller注册使用）
	int fd() const;

	//Getter:检查套接字句柄是否有效（m_fd >= 0）
	bool valid() const;

private:
	int m_fd; //内核套接字句柄
};

#endif 


