#include "Connection.h"
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include<iostream>

static const int BUF_SIZE = 1024;

// 构造函数：初始化 fd、状态、Channel，并绑定回调
Connection::Connection(int fd)
    : m_fd(fd)
    , m_closed(false)
    , m_channel(std::make_unique<Channel>(fd))
{
    // 默认关心可读事件
    m_channel->enableRead();

    // 把 Connection 的成员函数注册为 Channel 的回调
    m_channel->setReadCallback([this]() { this->handleRead(); });
    m_channel->setWriteCallback([this]() { this->handleWrite(); });
    m_channel->setErrorCallback([this]() { this->m_closed = true; });
}

// 析构函数：如果连接还没关闭，自动关闭 fd
Connection::~Connection() {
    if (!m_closed) {
        ::close(m_fd);
    }
}

// 处理可读事件：读取数据、解析 HTTP 请求、生成响应
void Connection::handleRead() {
    if (m_closed) return;

    char buf[BUF_SIZE];
    while (true) {
        ::memset(buf, 0, sizeof(buf));
        ssize_t bytesRead = ::read(m_fd, buf, sizeof(buf) - 1);

        if (bytesRead > 0) {
            // 1. 把读到的原始数据追加到读缓冲区
            m_readBuffer.append(buf, bytesRead);

            // 2. 只要缓冲区还有数据，就尝试解析
            //    这样即使请求体数据在后续才到达，也能继续处理
            while (!m_readBuffer.empty()) {
                if (m_parser.parse(m_readBuffer, m_request)) {
                    // 3. 解析完成，生成 HTTP 响应
                    std::string response;
                    response += "HTTP/1.1 200 OK\r\n";
                    response += "Content-Type: text/plain\r\n";
                    std::string body = "Hello, World!";
                    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
                    response += "\r\n";
                    response += body;

                    // 4. 把响应追加到写缓冲区，而不是直接覆盖，
                    //    防止之前未发送完的数据丢失
                    m_writeBuffer += response;
                    handleWrite();

                    // 5. 重置请求对象，准备解析下一个请求（Keep-Alive 基础）
                    m_request.clear();
                } else {
                    // 6. 数据不足，退出解析循环，等待下一次 read
                    break;
                }
            }
        } else if (bytesRead == 0) {
            // 客户端正常断开连接
            printf("Client[%d] disconnected.\n", m_fd);
            m_closed = true;
            break;
        } else {
            // 读取出错
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // ET 模式下缓冲区已读空，正常退出
                break;
            } else if (errno == EINTR) {
                // 被信号中断，继续重试
                continue;
            } else {
                // 真正的读取错误
                perror("read error");
                m_closed = true;
                break;
            }
        }
    }
}

// 处理可写事件：把写缓冲区中的数据发送出去
void Connection::handleWrite() {
    if (m_closed || m_writeBuffer.empty()) return;

    size_t total = 0; // 记录已经成功发送的字节数
    while (total < m_writeBuffer.size()) {
        ssize_t bytesWritten = ::write(m_fd,
                                       m_writeBuffer.data() + total,
                                       m_writeBuffer.size() - total);
        if (bytesWritten > 0) {
            total += bytesWritten;
        } else if (bytesWritten < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 发送缓冲区满，暂时退出，等待 EPOLLOUT 事件
                break;
            } else if (errno == EINTR) {
                // 被信号中断，继续重试
                continue;
            } else {
                // 真正的写入错误
                perror("write error");
                m_closed = true;
                break;
            }
        }
    }

    // 删除已经发送的数据，保留未发送完的部分
    if (total > 0) {
        m_writeBuffer.erase(0, total);
    }
}

// 判断连接是否已关闭
bool Connection::isClosed() const {
    return m_closed;
}

// 获取底层文件描述符
int Connection::fd() const {
    return m_fd;
}

// 获取 Channel 指针，供 Reactor 使用
Channel* Connection::getChannel() {
    return m_channel.get();
}
