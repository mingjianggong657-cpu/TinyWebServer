#ifndef CONNECTION_H
#define CONNECTION_H

#include<string>
#include<memory>
#include"Channel.h"
#include"HttpParser.h"
#include"HttpRequest.h"
class Epoller;

class Connection {
public:
	//构造函数
	explicit Connection(int fd);

        //析构函数
        ~Connection();

        //禁止拷贝，允许移动
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        //处理可读事件：读取数据+Echo回写
        void handleRead();

        //处理可写事件
        void handleWrite();

	//检查连接是否已关闭（对端断开或出错）
	bool isClosed() const;

       //获取底层fd(供Epoller使用)
       int fd() const;

       //暴露Channel供Reactor使用（只给使用权，不给所有权）
       Channel* getChannel();

       void setEpoller(Epoller* epoller);

       //追加数据到读缓冲区（handleRead时调用）
       void appendToReadBuffer(const char* data,size_t len) {
         m_readBuffer.append(data,len);
       }

       //获取读缓冲区的引用（HttpParser需要读取和消费数据）
       std::string& readBuffer() {return m_readBuffer;}

private:
       int m_fd; //客户端连接的套接字句柄
       bool m_closed; //标记连接是否已关闭
       std::string m_writeBuffer; //待发送的数据
       std::unique_ptr<Channel> m_channel; //Connection独占Channel的所有权
       Epoller* m_epoller;  //用于同步内核epoll事件
       bool m_closeAfterWrite; //标记：响应发送完成后是否关闭连接
       std::string m_readBuffer; //暂存从客户端读到的原始字节流
       HttpParser m_parser;
       HttpRequest m_request;
};


#endif
