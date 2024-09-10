#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__
#include "debug_service.h"
#include "at_socket.h"
#include "example_tcp_server.h"

static void example_tcp_server_proc_incoming_client(void *argument)
{
    int ret;
    socket_tcp_server_config *config = (socket_tcp_server_config *)argument;
    unsigned int cli_sock_fd = config->sock_fd;
    unsigned int loop_count = config->loop_count;
    unsigned int loop_interval = config->loop_interval;  
    char buf[64];

    LOG_V("%s Start(%d)",__FUNCTION__, cli_sock_fd);	

    for (int i=0; i<loop_count; i++)
	{
		memset(buf,0,64);
		ret = recv(cli_sock_fd,buf,sizeof(buf),0);	
        if (0 < ret)	
            LOG_I("Tcp server recv %s ok", buf);
        else
        {
            LOG_E("Tcp server recv %s err, %d", buf, cli_sock_fd);
            break;
        }
        ret = send(cli_sock_fd,buf,strlen(buf),0);
        if (ret > 0)
            LOG_I("Tcp server send %s ok %d", buf, ret);
        else
        {
            LOG_E("Tcp server send %s err %d", buf, ret);
            break;
        }
        osDelay(loop_interval);
    } 
    closesocket(cli_sock_fd); 
    
    LOG_V("%s over",__FUNCTION__);	
    os_thread_exit();
}

int example_tcp_server_test(short sin_port, char *sin_addr, int max_connect_num, int loop_count, int loop_interval)
{
    int ser_sock_fd, i, ret, cli_sock_fd, addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in ser_sock_addr, cli_sock_addr;
    socket_tcp_server_config config;

    osThreadId_t thread_id =NULL;
    osThreadAttr_t thread_attr = {.stack_size = 256*12, .priority = osPriorityNormal};

    LOG_V("%s Start",__FUNCTION__);	

    //1.creat socket(ipv4 tcp)
    ser_sock_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(ser_sock_fd<0)
    {
        LOG_E("Socket creat err %d", ser_sock_fd);
        return -1;
    }

    //2.bind server socket
    ser_sock_addr.sin_family =  AF_INET;                //IPV4
    ser_sock_addr.sin_port   =  htons(sin_port);        //port    
    ser_sock_addr.sin_addr.s_addr  = inet_addr(sin_addr);  //ip
    ret = bind(ser_sock_fd, (struct sockaddr *)&ser_sock_addr, addr_len);  
    if(ret == -1)
    {
        LOG_E("Server bind failure");
        return -1;
    }
    LOG_I("Server bind success");

    for (i=0; i<max_connect_num; i++)
    {
    //3. listen
        ret = listen(ser_sock_fd, 0); //The second parameter is temporarily invalid
        if(ret == -1)
        {
            LOG_E("Server listen failure");
            return -1;
        } 

    //4. accept
        cli_sock_fd = accept(ser_sock_fd, (struct sockaddr*)(&cli_sock_addr), &addr_len);
        if(cli_sock_fd == -1)
        {
            LOG_E("Server accept failure");
            return -1;
        }
        LOG_I("New client connect(%s, %d), %d", inet_ntoa(cli_sock_addr.sin_addr.s_addr), cli_sock_addr.sin_port, cli_sock_fd);

    //5. create net service
        config.loop_count = loop_count;
        config.loop_interval = loop_interval;
        config.sock_fd = cli_sock_fd;
        thread_id = os_thread_create(example_tcp_server_proc_incoming_client, (void *)&config, &thread_attr);
        if (NULL == thread_id)
        {
            LOG_E ("thread_id thread could not start!");
            return -1;
        }
        LOG_I("%s (%x)",__FUNCTION__, thread_id);	
    }
    closesocket(ser_sock_fd);  

    LOG_D("%s over",__FUNCTION__);	

    return 0;
}

#endif  /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_SOCKET_TCP_SERVER_EXAMPLE__ */