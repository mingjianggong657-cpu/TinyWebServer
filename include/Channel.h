#ifndef CHANNEL_H
#define CHANNEL_H

#include<functional>
#include<cstdint>

using EventCallback = std::function<void()>;

class Channel {
public:
       //构造函数：Channel只观察fd,不拥有fd
       explicit Channel(int fd);
       ~Channel() = default;

       //设置回调函数：由Connection在初始化时注册
       void setReadCallback(EventCallback cb);
       void setWriteCallback(EventCallback cb);
       void setErrorCallback(EventCallback cb);

       //事件到来时由Reactor调用,内部根据events分发到对应回调
       void handleEvent(uint32_t events);

       //修改Channel关心的事件类型（只改m_events,不调用epoll_ctl）
       void enableRead();  //m_events |= EPOLLIN
       void enableWrite(); //m_events |= EPOLLOUT
       void disableWrite(); //只清除EPOLLOUT标志，保留EPOLLIN等其他事件
       void disableAll(); //m_events = 0

       //Getters
       int fd() const;
       uint32_t events() const;

private:
       int m_fd; //关联的文件描述符
       uint32_t m_events; //当前关心的文件类型
       
       EventCallback m_readCallback; //EPOLLIN时调用
       EventCallback m_writeCallback; //EPOLLOUT时调用
       EventCallback m_errorCallback; //出错时调用
};

#endif

