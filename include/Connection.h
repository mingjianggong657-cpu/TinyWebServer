#ifndef CONNECTION_H
#define CONNECTION_H

#include<string>

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

private:
       int m_fd; //客户端连接的套接字句柄
       bool m_closed; //标记连接是否已关闭
       std::string m_writeBuffer; //待发送的数据
};


#endif
