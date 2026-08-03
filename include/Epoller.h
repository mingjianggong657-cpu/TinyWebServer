#ifndef EPOLLER_H
#define EPOLLER_H

#include<sys/epoll.h>
#include<unistd.h>
#include<vector>
#include<cassert>


class Epoller{

public:
	//构造函数：初始化epoll实例，并预分配events数组大小(防止隐式转换）
	explicit Epoller(int maxEvents = 1024);

	//析构函数：负责彻底释放m_epollFd资源（RAII）
	~Epoller();

	//禁用拷贝构造和赋值（内核资源独占，防止多次释放）
	Epoller(const Epoller&) = delete;
	Epoller& operator=(const Epoller&) = delete;

	//封装epoll_ctl添加节点
	bool addFd(int fd,uint32_t events);

	//封装epoll_ctl修改节点
	bool modFd(int fd,uint32_t events);

	//封装epoll_ctl删除节点
	bool delFd(int fd);

	//封装epoll_wait等待就绪事件
	int wait(int timeoutMs = -1);

	//辅助函数：获取第i个就绪事件的fd
	int getEventFd(size_t i) const;

	//辅助函数：获取第i个就绪事件的事件类型
	uint32_t getEvents(size_t i) const;


private:
	int m_epollFd; //内核epoll实例句柄
	std::vector<struct epoll_event> m_events; //存储epoll_wait返回的就绪事件列表
};


#endif


