#include "reactor.h"

using namespace std;

int epfd = 0;               //作为全局变量
Conne conn_poll[CONNECTIONS_SIZE] = {0};

//listen(socketfd)-------> EPOLLIN------------->Accept_cb
int Accept_cb(int fd)
{
    struct sockaddr_in clientaddr; //创建一个客户端用于接收数据
    socklen_t len = sizeof(clientaddr);

    cout<<"current socketfd:"<<fd<<endl;
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
    cout<<"clientfd:"<<clientfd<<endl;       //获取到客户端的连接描述符
    if (clientfd < 0){
        cout<<"accept error:"<<strerror(errno)<<endl;
        return -1;
    }
    EventRegister(clientfd, EPOLLIN); //监听可读事件

    return 0;
}

int Recv_cb(int fd)
{
    int count = recv(fd, conn_poll[fd].rbuffer, BUFFER_SIZE, 0);
    if (count == 0)
    {
        cout << "client close" << endl;
        close(conn_poll[fd].fd);                                // 关闭客户端的连接描述
        epoll_ctl(fd, EPOLL_CTL_DEL, conn_poll[fd].fd, NULL); // 将客户端的连接描述符从epoll实例中删除
        return 0;
    }
    cout << "recv_buffer:" << conn_poll[fd].rbuffer << endl;

    conn_poll[fd].wlen = count;
    memcpy(conn_poll[fd].wbuffer, conn_poll[fd].rbuffer, count);

    SetEvent(fd, EPOLLOUT,0); //监听可写事件

    return count;
}

int Send_cb(int fd)
{
    // 返回信息
    int count = send(fd, conn_poll[fd].wbuffer, conn_poll[fd].wlen, 0);

    SetEvent(fd, EPOLLIN,0); //监听可读事件

    return count;
}

int SetEvent(int fd, int event, int flag)
{
    if (flag)
    { // 添加事件
        struct epoll_event ev;
        ev.events = event;                     // 监听可读事件
        ev.data.fd = fd;                         // 监听描述符
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev); // 将监听描述符加入到epoll实例中
    }
    else
    { // 修改事件
        struct epoll_event ev;
        ev.events = event;                     // 监听可读事件
        ev.data.fd = fd;                         // 监听描述符
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev); // 将监听描述符加入到epoll实例中
    }
    return 0;
}

int Init_Server(unsigned short port)
{
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //绑定0.0.0.0，Linux上有三张网卡,eth0:桥接,eth1：NAT,lo:回环,
                                                  // 0.0.0.0代表任意一个收到数据都行，也可以绑定固定网卡上的ip地址
    servaddr.sin_port = htons(port);              // 0~1023:系统使用,1024以后用户可以使用
    if (-1 == bind(socketfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)))
    {
        cout << "bind failed:" << strerror(errno) << endl;
    }

    listen(socketfd, 10); //监听
    cout<<"socketfd is "<<socketfd<<endl;

    return socketfd;
}

int EventRegister(int fd, int event)
{
    if(fd < 0 || fd >= CONNECTIONS_SIZE){
        cout<<"fd is invalid"<<endl;
        return -1;
    }

    //
    conn_poll[fd].fd = fd;
    conn_poll[fd].recv_or_accept.accept_callback = Recv_cb;
    conn_poll[fd].send_callback = Send_cb;

    //防止复用，所以对缓存清0
    memset(conn_poll[fd].rbuffer, 0, BUFFER_SIZE);
    conn_poll[fd].rlen = 0;

    memset(conn_poll[fd].wbuffer, 0, BUFFER_SIZE);
    conn_poll[fd].wlen = 0;

    SetEvent(fd, event, 1); //监听客户端的可读事件

    return 0;
}

int main()
{
    // 初始化服务器
    unsigned short port = 2000;
    int socketfd = Init_Server(port);

    epfd = epoll_create(1024); //创建一个epoll实例，参数是监听描述符的数量

    conn_poll[socketfd].fd = socketfd;
    conn_poll[socketfd].recv_or_accept.accept_callback = Accept_cb; //设置回调函数，当有连接到来时调用Accept函数处理连接请求


    SetEvent(socketfd, EPOLLIN,1);   //监听socketfd上的可读事件

    while(true){
        struct epoll_event events[1024]={0};
        int nready = epoll_wait(epfd, events, 1024, -1); //等待IO就绪
        if(nready < 0){                //出错处理
            cout << "reactor error:" << strerror(errno) << endl;
            continue;
        }
        for(int i = 0; i < nready; i++){
            if(events[i].events & EPOLLIN){           //判断IO是否可读
                conn_poll[events[i].data.fd].recv_or_accept.recv_callback(events[i].data.fd);
            }
            //别写成else if，需要关注可读和可写事件同时发生的情况
            if(events[i].events & EPOLLOUT){         //判断IO是否可写
                conn_poll[events[i].data.fd].send_callback(events[i].data.fd);
            }
        }
    }

    return 0;
}
