#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include<unordered_map>
#include<vector>

#include "Connection.h"
#include "Socket.h"
#include "Epoller.h"
#include "HeapTimer.h"

const uint16_t PORT = 8888;

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
	HeapTimer timer;
	const int TIMEOUT_MS = 30000; //30秒无活动关闭连接
	std::unordered_map<int,Connection*> connections;
	epoller.addFd(serverSock.fd(), EPOLLIN | EPOLLET);

	// 3. 事件大循环 (Event Loop)
	while (true) {
		//获取下一个超时事件，传给epoll_wait作为超时参数
		int timeoutMs = timer.getNextTimeout();
		int eventCnt = epoller.wait(timeoutMs);
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

					Connection* conn = new Connection(clientFd);
					conn->setEpoller(&epoller);
					connections[clientFd] = conn;

					Channel* channel = conn->getChannel();
					epoller.addFd(channel->fd(),channel->events() | EPOLLET);
                                        //新连接加入定时器
      					timer.addTimer(clientFd,TIMEOUT_MS);

					std::cout << "Client [" << clientFd << "] connected." << std::endl;
				}
			} 
			// 情况 B：可读事件发生
			else {
				auto it = connections.find(eventFd);
				if(it != connections.end()) {
                                        Connection* conn = it->second;
					conn->getChannel()->handleEvent(events);
                                        
					//发生了读写事件，刷新定时器
                                        if(!conn->isClosed() && (events & (EPOLLIN | EPOLLOUT))) {
      timer.adjustTimer(eventFd,TIMEOUT_MS);
					}
 


					if(conn->isClosed())
					{
						epoller.delFd(eventFd);
						connections.erase(it);
						timer.removeTimer(eventFd);
						delete conn;
						std::cout << "Client [" << eventFd << "] connection cleaned up." << std::endl;
					}
				}
			}
		}
                     //处理超时连接
		     std::vector<int> expiredFds = timer.tick();
		     for(int fd : expiredFds) {
                        auto it = connections.find(fd);
			if(it != connections.end()) {
                          Connection* conn = it->second;
			  epoller.delFd(fd);
			  connections.erase(it);
			  delete conn;
			  std::cout << "Client [" << fd << "] timeout,connection cleaned up." << std::endl;
			}
		     }

	}

	return 0;
}
