#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<cerrno>
#include<csignal>

//设置非阻塞
int setnonblocking(int fd)
{
        int flag = fcntl(fd,F_GETFL);
	if(flag == -1) return -1;
	flag |= O_NONBLOCK;
        return fcntl(fd,F_SETFL,flag);
}


int main()
{
   //第一步：创建监听套接字
   int listen_fd = socket(AF_INET,SOCK_STREAM,0);
   setnonblocking(listen_fd);

   //第二步：绑定IP和端口
   struct sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(8888); //端口号
   addr.sin_addr.s_addr = INADDR_ANY;  //监听所有IP
   bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));

   //第三步：开始监听
   listen(listen_fd, 5);
   
   //创建epoll实例
   int epoll_fd = epoll_create(1);

   //定义epoll事件结构体，准备把listen_fd挂载上去
   struct epoll_event ev;
   ev.events = EPOLLIN;//监听读事件
   ev.data.fd = listen_fd;
   epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&ev);
   
   //用于存放内核返回的就绪事件数组
   struct epoll_event events[1024];
   
   //进入服务器的主事件循环,持续监测
   while(true)
   {
	   //调用一次，检测一次
	   int num = epoll_wait(epoll_fd,events,1024,-1);
           for(int i=0;i<num;i++)
	   {
              //取出当前的文件描述符
	      int curfd = events[i].data.fd;
	      //判断这个文件描述符是不是用于监听的
	      if(curfd == listen_fd)
	      {
                //建立新的连接
		int cfd = accept(curfd,NULL,NULL);
		setnonblocking(cfd); //设置非阻塞
		//新得到的文件描述符添加到epoll模型中，下一轮循环的时候就可以被检测了
		ev.events = EPOLLIN | EPOLLET; //读缓冲区是否有数据
		ev.data.fd = cfd;
		setnonblocking(cfd);
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,cfd,&ev);
		printf("新客户端连接成功,fd = %d\n",cfd);
	      }
	      else
	      {
              //处理通信的文件描述符
	       //接收数据
	       char buf[1024];
	       while(1)
	       {
                memset(buf,0,sizeof(buf));
		int len = recv(curfd,buf,sizeof(buf)-1,0);

		if(len > 0)
		{
			printf("客户端(fd = %d) say: %s\n",curfd,buf);
			send(curfd,buf,len,0);
		}
		else if(len == 0)
		{
			printf("客户端(fd = %d)已经断开连接\n",curfd);
			epoll_ctl(epoll_fd,EPOLL_CTL_DEL,curfd,NULL);
			close(curfd);
			break;
		}
		else
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break;
			}
			else if(errno == EINTR)
			{
			        continue;
			}
			else
			{
			        perror("recv error");
				epoll_ctl(epoll_fd,EPOLL_CTL_DEL,curfd,NULL);
				close(curfd);
				break;
			}


		}

	       }


	      }

	   }

   }   
   close(listen_fd);
   return 0;
}

