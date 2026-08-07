#include "Connection.h"
#include<unistd.h>
#include<cstring>
#include<cstdio>
#include<cerrno>

static const int BUF_SIZE = 1024;

Connection::Connection(int fd) : m_fd(fd),m_closed(false){}

Connection::~Connection(){
       if(!m_closed){
           ::close(m_fd);
       }
}

void Connection::handleRead() {
       if(m_closed) return;

       char buf[BUF_SIZE];
      while(true) {
          ::memset(buf,0,sizeof(buf));
	  ssize_t bytesRead = ::read(m_fd,buf,sizeof(buf)-1);

	  if(bytesRead > 0) {
            //只负责读取，数据暂存到writeBuffer
	    printf("Client[%d]: %s",m_fd,buf);
	    m_writeBuffer.append(buf,bytesRead);
	   }
	  else if(bytesRead == 0)
	  {
             //客户端正常断开
	     printf("Client[%d] disconnected.\n",m_fd);
	     m_closed = true;
	     break;
	  }
	  else
	  {
           if(errno == EAGAIN || errno == EWOULDBLOCK)
	   {
             //ET模式缓冲区已读空，退出循环
	     break;
	   }
	   else if(errno == EINTR)
	   {
             //被信号中断，继续重试
              continue;
	   }
	   else
	   {
		   //真正的读取出错
		   perror("read error");
		   m_closed = true;
		   break;
	   }
	  }

      } 
}


void Connection::handleWrite(){
    if(m_closed || m_writeBuffer.empty()) return;

    size_t total = 0; //记录已经成功发送出去的字节数
    while(total < m_writeBuffer.size()) {
         ssize_t bytesWritten = ::write(m_fd,
			 m_writeBuffer.data()+total,
			 m_writeBuffer.size()-total);
	 if(bytesWritten > 0) {
	  total += bytesWritten;
	 }
	 else if(bytesWritten < 0) {
           if(errno == EAGAIN || errno == EWOULDBLOCK) {
            //发送缓冲区满，暂时退出，等待下次EPOLLOUT
	    break;  
	   }
	   else if(errno == EINTR) {
              //被信号中断，继续重试
	      continue;
	   }
	   else{
             perror("write error");
	     m_closed = true;
             break;
	   }
	 }
    }

    //移除已发送的数据
    if(total > 0){
       m_writeBuffer.erase(0,total);
    }

}

bool Connection::isClosed() const {
 return m_closed;
}

int Connection::fd() const {
  return  m_fd;
}
