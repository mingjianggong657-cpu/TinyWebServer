#include "Epoller.h"
#include<unistd.h>
#include<cstdio>

//构造函数：初始化epoll句柄并预分配events数组空间
Epoller::Epoller(int maxEvents)
	: m_epollFd(epoll_create(1)),m_events(maxEvents){
          assert(m_epollFd >= 0 && m_events.size() > 0);
	}

//析构函数：自动关闭内核epoll句柄（RAII）
Epoller::~Epoller(){
    if(m_epollFd >= 0){
      close(m_epollFd);
    }
}

//封装epoll_ctl添加节点
bool Epoller::addFd(int fd,uint32_t events){
        if(fd < 0) return false;
	struct epoll_event ev = {0};
	ev.events = events;
	ev.data.fd = fd;
        return 0 == epoll_ctl(m_epollFd,EPOLL_CTL_ADD,fd,&ev);
}

//封装epoll_ctl修改节点
bool Epoller::modFd(int fd,uint32_t events){
        if(fd < 0) return false;
        struct epoll_event ev = {0};
        ev.events = events;
        ev.data.fd = fd;
        return 0 == epoll_ctl(m_epollFd,EPOLL_CTL_MOD,fd,&ev);

}

//封装epoll_ctl删除节点
bool Epoller::delFd(int fd){
       if(fd < 0) return false;
       return 0 == epoll_ctl(m_epollFd,EPOLL_CTL_DEL,fd,nullptr);
}

//封装epoll_wait等待就绪事件
int Epoller::wait(int timeoutMs){
    return epoll_wait(m_epollFd,&m_events[0],static_cast<int>(m_events.size()),timeoutMs);
}

//辅助函数：获取第i个就绪事件的fd
int Epoller::getEventFd(size_t i) const {
     assert(i < m_events.size());
     return m_events[i].data.fd;
}

//辅助函数：获取第i个就绪事件的事件类型
uint32_t Epoller::getEvents(size_t i) const {
         assert(i < m_events.size());
         return m_events[i].events;
}
