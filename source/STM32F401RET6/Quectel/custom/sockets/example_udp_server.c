#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_SERVER_EXAMPLE__
#include "debug_service.h"
#include "at_socket.h"
#include "example_tcp_server.h"

int example_udp_server_test(short sin_port, char *sin_addr, int loop_count, int loop_interval)
{
    int ser_sock_fd, ret, addr_len = sizeof(struct sockaddr_in);
    char buf[1024] = "hello";
    struct sockaddr_in ser_sock_addr, cli_sock_addr;

    LOG_V("%s Start",__FUNCTION__);	

    //1.creat socket(ipv4 udp)
    ser_sock_fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if(ser_sock_fd<0)
    {
        LOG_E("Socket creat err %d", ser_sock_fd);
        return -1;
    }

    //2.bind server socket
    ser_sock_addr.sin_family =  AF_INET;                   //IPV4
    ser_sock_addr.sin_port   =  htons(sin_port);           //port    
    ser_sock_addr.sin_addr.s_addr  = inet_addr(sin_addr);  //ip   
    ret = bind(ser_sock_fd, (struct sockaddr *)&ser_sock_addr, addr_len);  
    if(ret == -1)
    {
        LOG_E("Server bind failure");
        return -1;
    }
    LOG_I("Server bind success");

    //3.recvfrom/sendto
    for (int i=0; i<loop_count; i++)
    {
        memset(buf,0,64);
        ret = recvfrom(ser_sock_fd,buf,64,0,(struct sockaddr *)&cli_sock_addr,sizeof(cli_sock_addr));	
        if (0 < ret)	
            LOG_I("Udp server recv %s ok, ret = %d, fd = %d (from:%s, %d)", buf, ret, ser_sock_fd, inet_ntoa(cli_sock_addr.sin_addr.s_addr), cli_sock_addr.sin_port);

        else
            LOG_E("Udp server recv %s err", buf);

        ret = sendto(ser_sock_fd,buf,strlen(buf),0,(struct sockaddr *)&cli_sock_addr,sizeof(cli_sock_addr));
        if (ret > 0)
            LOG_I("Udp server send %s ok, ret = %d, fd = %d (to:%s, %d)", buf, ret, ser_sock_fd, inet_ntoa(cli_sock_addr.sin_addr.s_addr), cli_sock_addr.sin_port);
        else
            LOG_E("Udp server send %s err(to:%s, %d)", buf, inet_ntoa(cli_sock_addr.sin_addr.s_addr), cli_sock_addr.sin_port);
        
        osDelay(loop_interval);
    }
    closesocket(ser_sock_fd);  

    LOG_D("%s over",__FUNCTION__);	

    return 0;
}

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_SERVER_EXAMPLE__ */