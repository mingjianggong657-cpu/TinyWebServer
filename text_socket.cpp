#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

//设置非阻塞
int setnonblocking(int fd)
{
        int flag = fcntl(fd,F_GETFL);
	if(flag == -1) return -1;
	flag |= O_NONBLOCK;
        return fcnt(fd,F_SETFL,flag);
}


int main()
{
   //第一步：创建监听套接字
   int listen_fd = socket(AF_INET,SOCK_STREAM,0);
    etnonblocking(listen_fd);

   //第二步：绑定IP和端口
   struct sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(8888); //端口号
   addr.sin_addr.s_addr = INADDR_ANY;  //监听所有IP
   bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));

   //第三步：开始监听
   listen(listen_fd, 5);

   //第四步：等待客户端连接
   int client_fd = accept(listen_fd, nullptr,nullptr);

   //第五步：收发数据
   char buf[1024];
   recv(client_fd, buf, sizeof(buf), 0);   //收
   send(client_fd, buf, strlen(buf), 0);   // 发（echo回去）
   
   //第六步：关闭
   close(client_fd);
   close(listen_fd);
   return 0;
}

