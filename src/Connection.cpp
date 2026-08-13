#include "Connection.h"
#include<unistd.h>
#include<cstring>
#include<cstdio>
#include<cerrno>

static const int BUF_SIZE = 1024;

Connection::Connection(int fd) : m_fd(fd)
				 ,m_closed(false)
                                 ,m_channel(std::make_unique<Channel>(fd))
                                  {
				    //让Channel默认关心可读事件
                                    m_channel->enableRead();

				    //把Connection的成员函数绑定为Channel的回调
				    m_channel->setReadCallback([this](){this->handleRead();});
				    m_channel->setWriteCallback([this](){this->handleWrite( );});
				    m_channel->setErrorCallback([this](){this->m_closed = true;});
				  }

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
            //追加到读缓冲区
	    m_readBuffer.append(buf,bytesRead);

	    //驱动解析器，可能多次解析（如果有粘包）
	    while(m_parser.parse(m_readBuffer,m_request)) {
              //解析完成，生成简单的Http响应
	      std::string response;
	      response += "HTTP/1.1 200 OK\r\n";
	      response += "Content-Type: text/plain\r\n";
	      std::string body = "Hello,World!";
	      response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	      response += "\r\n";
	      response += body;

	      m_writeBuffer = std::move(response);
              handleWrite();  //立即发送（Echo阶段可接受）
              
	      //重置request,准备解析下一个请求(Keep-Alive 基础)
	      m_request.clear();
	    }
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

Channel* Connection::getChannel(){
  return m_channel.get();
}
