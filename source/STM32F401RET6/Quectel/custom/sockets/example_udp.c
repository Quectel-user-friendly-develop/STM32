#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_CLIENT_EXAMPLE__
#include "debug_service.h"
#include "at_socket.h"

int example_udp_client_test(short sin_port, char *sin_addr, int loop_count, int loop_interval)
{
    int fd, ret;
    char buf[64];
    struct sockaddr_in ser_sockaddr;
    
    LOG_I("%s Start",__FUNCTION__);	

    //1.creat socket(ipv4 tcp)
    fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if(fd<0)
    {
        LOG_E("Socket creat err");
        return -1;
    }

    //2. creat server addr
    ser_sockaddr.sin_family =  AF_INET;                 //IPV4
    ser_sockaddr.sin_port   =  htons(sin_port);         //port    
    ser_sockaddr.sin_addr.s_addr  = inet_addr(sin_addr);//ip

    sprintf(buf, "%d", fd);
    for (int i=0; i<loop_count; i++)
    {
        ret = sendto(fd,buf,strlen(buf),0,(struct sockaddr *)&ser_sockaddr,sizeof(ser_sockaddr));
        if (ret > 0)
            LOG_I("Udp client send %s ok, ret = %d, fd = %d (to:%s, %d)", buf, ret, fd, inet_ntoa(ser_sockaddr.sin_addr.s_addr), ntohs(ser_sockaddr.sin_port));   
        else
            LOG_E("Udp client send %s err %d, %d", buf, ret, fd);

        memset(buf,0,64);
        ret = recvfrom(fd,buf,64,0,(struct sockaddr *)&ser_sockaddr,sizeof(ser_sockaddr));	
        if (0 < ret)	
            LOG_I("Udp client recv %s ok, ret = %d, fd = %d (from:%s, %d)", buf, ret, fd, inet_ntoa(ser_sockaddr.sin_addr.s_addr), ntohs(ser_sockaddr.sin_port));   
        else
            LOG_E("Udp client recv %s err", buf);
        
        osDelay(loop_interval);
    }  
    closesocket(fd);  

    LOG_V("%s over",__FUNCTION__);	

    return 0;
}
#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_UDP_CLIENT_EXAMPLE__ */
