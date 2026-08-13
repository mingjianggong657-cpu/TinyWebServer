#include"Channel.h"
#include<sys/epoll.h>
#include<utility>

Channel::Channel(int fd)
	:m_fd(fd),m_events(0) {}

void Channel::setReadCallback(EventCallback cb) {
     m_readCallback = std::move(cb);
}

void Channel::setWriteCallback(EventCallback cb) {
     m_writeCallback = std::move(cb);
}

void Channel::setErrorCallback(EventCallback cb) {
     m_errorCallback = std::move(cb);
}

void Channel::handleEvent(uint32_t events) {
     //先处理错误和挂起事件
     if((events & EPOLLERR) || (events & EPOLLHUP) || (events & EPOLLRDHUP)) {
          if(m_errorCallback) m_errorCallback();
	  return;
     }
     //可读事件
     if(events & EPOLLIN) {
       if(m_readCallback) m_readCallback();
     }

     //可写事件
     if(events & EPOLLOUT) {
        if(m_writeCallback) m_writeCallback();
     }
}

void Channel::enableRead() {
      m_events |= EPOLLIN;
}

void Channel::enableWrite() {
      m_events |= EPOLLOUT;
}

void Channel::disableAll() {
      m_events = 0;
}

int Channel::fd() const {
      return m_fd;
}

uint32_t Channel::events() const {
      return m_events;
}
