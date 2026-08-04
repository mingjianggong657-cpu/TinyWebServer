#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

#include "Socket.h"
#include "Epoller.h"

const uint16_t PORT = 8888;
const int BUF_SIZE = 1024;

// 辅助函数：将客户端 fd 设置为非阻塞
void setNonBlocking(int fd) {
    int oldOption = ::fcntl(fd, F_GETFL);
    ::fcntl(fd, F_SETFL, oldOption | O_NONBLOCK);
}

int main() {
    // 1. 服务端监听 Socket：由 Socket 类进行 RAII 生命周期管理
    Socket serverSock;
    if (!serverSock.bind(PORT) || !serverSock.listen()) {
        std::cerr << "Server init error!" << std::endl;
        return 1;
    }
    serverSock.setNonBlocking();

    std::cout << "Server started on port " << PORT << "..." << std::endl;

    // 2. 注册监听套接字到 Epoller
    Epoller epoller(1024);
    epoller.addFd(serverSock.fd(), EPOLLIN | EPOLLET);

    // 3. 事件大循环 (Event Loop)
    while (true) {
        int eventCnt = epoller.wait(-1);
        if (eventCnt < 0) {
            if (errno == EINTR) continue; // 被信号打断，继续循环
            std::cerr << "epoll wait error!" << std::endl;
            break;
        }

        for (int i = 0; i < eventCnt; ++i) {
            int eventFd = epoller.getEventFd(i);
            uint32_t events = epoller.getEvents(i);

            // 情况 A：有新连接到来
            if (eventFd == serverSock.fd()) {
                while (true) {
                    int clientFd = serverSock.accept();
                    if (clientFd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // 所有就绪连接已全部处理完
                        }
                        perror("accept error");
                        break;
                    }

                    setNonBlocking(clientFd);
                    epoller.addFd(clientFd, EPOLLIN | EPOLLET);
                    std::cout << "Client [" << clientFd << "] connected." << std::endl;
                }
            } 
            // 情况 B：可读事件发生
            else if (events & EPOLLIN) {
                char buf[BUF_SIZE];
                while (true) {
                    ::memset(buf, 0, sizeof(buf));
                    ssize_t bytesRead = ::read(eventFd, buf, sizeof(buf) - 1);
                    
                    if (bytesRead < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // 本次缓冲区数据已全部读取完毕
                        }
                        std::cerr << "Read error on fd " << eventFd << std::endl;
                        epoller.delFd(eventFd);
                        ::close(eventFd);
                        break;
                    } 
                    else if (bytesRead == 0) {
                        // 客户端正常断开连接
                        std::cout << "Client [" << eventFd << "] disconnected." << std::endl;
                        epoller.delFd(eventFd);
                        ::close(eventFd);
                        break;
                    } 
                    else {
                        // Echo 回显并检查 write 返回值
                        std::cout << "Receive from [" << eventFd << "]: " << buf;
                        ssize_t bytesWritten = ::write(eventFd, buf, bytesRead);
                        if (bytesWritten < 0) {
                            std::cerr << "Write error on fd " << eventFd << std::endl;
                        }
                    }
                }
            }
        }
    }

    return 0;
}
