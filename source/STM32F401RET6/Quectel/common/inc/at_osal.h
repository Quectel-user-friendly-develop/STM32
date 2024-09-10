/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-12     armink       first version
 */

/*
 * NOTE: DO NOT include this file on the header file.
 */
#ifndef __AT_OSAL_H__
#define __AT_OSAL_H__

#include "stdio.h"
#include <stdarg.h>
#include "string.h"
#include <assert.h>
#include "cmsis_os2.h"
#include "debug_service.h"

/*********************************************User system type definitions****************************************/
#define AT_USING_SOCKET
#define AT_USING_CLIENT
#define ARCH_CPU_32BIT //ARCH_CPU_64BIT or ARCH_CPU_32BIT
#define AT_PRINT_RAW_CMD
#define LWIP_IPV6_SCOPES 1
#define NETDEV_IPV4 1

/*********************************************User system type definitions end*************************************/

/********************************************Basic data type definitions*******************************************/
typedef signed   char                   rt_int8_t;      /**<  8bit integer type */
typedef signed   short                  rt_int16_t;     /**< 16bit integer type */
typedef signed   int                    rt_int32_t;     /**< 32bit integer type */
typedef unsigned char                   rt_uint8_t;     /**<  8bit unsigned integer type */
typedef unsigned short                  rt_uint16_t;    /**< 16bit unsigned integer type */
typedef unsigned int                    rt_uint32_t;    /**< 32bit unsigned integer type */
typedef unsigned char                   u8_t;
typedef signed   char                   s8_t;
typedef unsigned short                  u16_t;
typedef signed   short                  s16_t;
typedef unsigned int                    u32_t;
typedef signed   int                    s32_t;

#ifdef ARCH_CPU_64BIT
typedef signed long                     rt_int64_t;     /**< 64bit integer type */
typedef unsigned long                   rt_uint64_t;    /**< 64bit unsigned integer type */
#else
typedef signed long long                rt_int64_t;     /**< 64bit integer type */
typedef unsigned long long              rt_uint64_t;    /**< 64bit unsigned integer type */
#endif

typedef int                             rt_bool_t;      /**< boolean type */
typedef long                            rt_base_t;      /**< Nbit CPU related date type */
typedef unsigned long                   rt_ubase_t;     /**< Nbit unsigned CPU related data type */

typedef rt_base_t                       rt_err_t;       /**< Type for error number */
typedef rt_uint32_t                     rt_time_t;      /**< Type for time stamp */
typedef rt_uint32_t                     rt_tick_t;      /**< Type for tick count */
typedef rt_base_t                       rt_flag_t;      /**< Type for flags */
typedef rt_ubase_t                      rt_size_t;      /**< Type for size number */
typedef rt_ubase_t                      rt_dev_t;       /**< Type for device */
typedef rt_base_t                       rt_off_t;       /**< Type for offset */

/* boolean type definitions */
#define RT_TRUE                         1               /**< boolean true  */
#define RT_FALSE                        0               /**< boolean fails */

#define TRUE                            1               /**< boolean true  */
#define FALSE                           0               /**< boolean fails */

/* RT-Thread error code definitions */
#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< No system */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */
#define RT_EINTR                        9               /**< Interrupted system call */
#define RT_EINVAL                       10              /**< Invalid argument */

/**
 * device flags defitions
 */
#define RT_DEVICE_OFLAG_CLOSE           0x000           /**< device is closed */
#define RT_DEVICE_OFLAG_RDONLY          0x001           /**< read only access */
#define RT_DEVICE_OFLAG_WRONLY          0x002           /**< write only access */
#define RT_DEVICE_OFLAG_RDWR            0x003           /**< read and write */
#define RT_DEVICE_OFLAG_OPEN            0x008           /**< device is opened */
#define RT_DEVICE_OFLAG_MASK            0xf0f           /**< mask of open flag */

#define RT_DEVICE_FLAG_INT_RX           0x100           /**< INT mode on Rx */
#define RT_DEVICE_FLAG_DMA_RX           0x200           /**< DMA mode on Rx */
#define RT_DEVICE_FLAG_INT_TX           0x400           /**< INT mode on Tx */
#define RT_DEVICE_FLAG_DMA_TX           0x800           /**< DMA mode on Tx */

/**
 * IPC flags and control command definitions
 */
#define RT_IPC_FLAG_FIFO                0x00            /**< FIFOed IPC. @ref IPC. */
#define RT_IPC_FLAG_PRIO                0x01            /**< PRIOed IPC. @ref IPC. */

#define RT_IPC_CMD_UNKNOWN              0x00            /**< unknown IPC command */
#define RT_IPC_CMD_RESET                0x01            /**< reset IPC object */

#define RT_WAITING_FOREVER              -1              /**< Block forever until get resource. */
#define RT_WAITING_NO                   0               /**< Non-block. */

#define RT_NULL                         (0)

/**
 * flag defintions in event
 */
#define RT_EVENT_FLAG_OR                0x00000000U            /**< logic or */
#define RT_EVENT_FLAG_AND               0x00000001U            /**< logic and */
#define RT_EVENT_FLAG_NO_CLEAR          0x00000002U            /**< no clear flag */
/********************************************Basic data type definitions end****************************************/

/********************************************Socket type definitions ***********************************************/
#define RT_NAME_MAX                     (16)
#define NETDEV_DNS_SERVERS_NUM          2U
#define AF_AT                           45  /* AT socket */
#define AT_DEVICE_NAMETYPE_NETDEV       0x02
#define AF_INET                         2

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM                     1
#define SOCK_DGRAM                      2
#define SOCK_RAW                        3

/* Flags we can use with send and recv. */
#define MSG_PEEK       0x01    /* Peeks at an incoming message */
#define MSG_WAITALL    0x02    /* Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define MSG_OOB        0x04    /* Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define MSG_DONTWAIT   0x08    /* Nonblocking i/o for this operation only */
#define MSG_MORE       0x10    /* Sender will send more */
/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define  SOL_SOCKET  0xfff    /* options for socket level */

#define AF_UNSPEC       0
#define AF_INET         2
#define AF_INET6        10
#define PF_INET         AF_INET
#define PF_UNSPEC       AF_UNSPEC

#define IPPROTO_IP      0
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#define IPPROTO_UDPLITE 136
/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF    0x1001    /* Unimplemented: send buffer size */
#define SO_RCVBUF    0x1002    /* receive buffer size */
#define SO_SNDLOWAT  0x1003    /* Unimplemented: send low-water mark */
#define SO_RCVLOWAT  0x1004    /* Unimplemented: receive low-water mark */
#define SO_SNDTIMEO  0x1005    /* Unimplemented: send timeout */
#define SO_RCVTIMEO  0x1006    /* receive timeout */
#define SO_ERROR     0x1007    /* get error status and clear */
#define SO_TYPE      0x1008    /* get socket type */
#define SO_CONTIMEO  0x1009    /* Unimplemented: connect timeout */
#define SO_NO_CHECK  0x100a    /* don't create UDP checksum */

/*
 * Options for level IPPROTO_TCP
 */
#define TCP_NODELAY    0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE  0x02    /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE   0x03    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL  0x04    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT    0x05    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */

/** DNS maximum host name length supported in the name table. */
#define DNS_MAX_NAME_LENGTH             256

#define EAI_NONAME      200
#define EAI_SERVICE     201
#define EAI_FAIL        202
#define EAI_MEMORY      203
#define EAI_FAMILY      204

#define HOST_NOT_FOUND  210
#define NO_DATA         211
#define NO_RECOVERY     212
#define TRY_AGAIN       213

/* input flags for structure addrinfo */
#define AI_PASSIVE      0x01
#define AI_CANONNAME    0x02
#define AI_NUMERICHOST  0x04
#define AI_NUMERICSERV  0x08
#define AI_V4MAPPED     0x10
#define AI_ALL          0x20
#define AI_ADDRCONFIG   0x40

/** Copy IP address - faster than ip_addr_set: no NULL check */
#define ip_addr_copy(dest, src) ((dest).addr = (src).addr)
#define ip_addr_cmp(addr1, addr2) ((addr1)->addr == (addr2)->addr)
#define rt_inline      static __inline

#define lwip_in_range(c, lo, up)  ((u8_t)(c) >= (lo) && (u8_t)(c) <= (up))
#define lwip_isdigit(c)           lwip_in_range((c), '0', '9')
#define lwip_isxdigit(c)          (lwip_isdigit(c) || lwip_in_range((c), 'a', 'f') || lwip_in_range((c), 'A', 'F'))
#define lwip_islower(c)           lwip_in_range((c), 'a', 'z')
#define lwip_isspace(c)           ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || (c) == '\t' || (c) == '\v')
#define lwip_isupper(c)           lwip_in_range((c), 'A', 'Z')
#define lwip_tolower(c)           (lwip_isupper(c) ? (c) - 'A' + 'a' : c)
#define lwip_toupper(c)           (lwip_islower(c) ? (c) - 'a' + 'A' : c)

typedef struct ip4_addr
{
    uint32_t addr;
} ip4_addr_t;

typedef uint32_t in_addr_t;
struct in_addr
{
    in_addr_t s_addr;
};

#define inet_aton(cp, addr)   osal_ip4addr_aton(cp,(ip4_addr_t*)addr)
#define inet_ntoa(addr)       ipaddr_ntoa((ip_addr_t*)&(addr))
/** IPv4 only: set the IP address given as an u32_t */
#define ip4_addr_set_u32(dest_ipaddr, src_u32) ((dest_ipaddr)->addr = (src_u32))

typedef unsigned int socklen_t;
typedef ip4_addr_t ip_addr_t;

struct sockaddr {
  u8_t sa_len;
  u8_t sa_family;
  char sa_data[14];
};

/* members are in network byte order */
struct sockaddr_in {
  u8_t sin_len;
  u8_t sin_family;
  u16_t sin_port;
  struct in_addr sin_addr;
  char sin_zero[8];
};
typedef uint8_t sa_family_t;
struct sockaddr_storage
{
    uint8_t        s2_len;
    sa_family_t    ss_family;
    char           s2_data1[2];
    uint32_t       s2_data2[3];
#if NETDEV_IPV6
    uint32_t       s2_data3[3];
#endif /* NETDEV_IPV6 */
};

struct hostent {
    char  *h_name;      /* Official name of the host. */
    char **h_aliases;   /* A pointer to an array of pointers to alternative host names,
                           terminated by a null pointer. */
    int    h_addrtype;  /* Address type. */
    int    h_length;    /* The length, in bytes, of the address. */
    char **h_addr_list; /* A pointer to an array of pointers to network addresses (in
                           network byte order) for the host, terminated by a null pointer. */
#define h_addr h_addr_list[0] /* for backward compatibility */
};

struct addrinfo {
    int               ai_flags;      /* Input flags. */
    int               ai_family;     /* Address family of socket. */
    int               ai_socktype;   /* Socket type. */
    int               ai_protocol;   /* Protocol of socket. */
    unsigned int      ai_addrlen;    /* Length of socket address. */
    struct sockaddr  *ai_addr;       /* Socket address of socket. */
    char             *ai_canonname;  /* Canonical name of service location. */
    struct addrinfo  *ai_next;       /* Pointer to next in list. */
};

/* These macros should be calculated by the preprocessor and are used
   with compile-time constants only (so that there is no little-endian
   overhead at runtime). */
#define PP_HTONS(x) ((((x) & 0x00ffUL) << 8) | (((x) & 0xff00UL) >> 8))
#define PP_NTOHS(x) PP_HTONS(x)
#define PP_HTONL(x) ((((x) & 0x000000ffUL) << 24) | \
                     (((x) & 0x0000ff00UL) <<  8) | \
                     (((x) & 0x00ff0000UL) >>  8) | \
                     (((x) & 0xff000000UL) >> 24))
#define PP_NTOHL(x) PP_HTONL(x)

#define htons(x) (uint16_t)PP_HTONS(x)
#define ntohs(x) (uint16_t)PP_NTOHS(x)
#define htonl(x) (uint32_t)PP_HTONL(x)
#define ntohl(x) (uint32_t)PP_NTOHL(x)

#define inet_addr(cp)         ipaddr_addr(cp)
/** IPv4 only: get the IP address as an u32_t */
#define ip4_addr_get_u32(src_ipaddr) ((src_ipaddr)->addr)
/** 255.255.255.255 */
#define IPADDR_NONE         ((u32_t)0xffffffffUL)
/********************************************Socket type definitions end********************************************/

/********************************************Slist type definitions ************************************************/
#define RT_SLIST_OBJECT_INIT(object) { RT_NULL }
/**
 * rt_slist_for_each - iterate over a single list
 * @pos:    the rt_slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define rt_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != RT_NULL; pos = pos->next)

/**
 * rt_container_of - return the member address of ptr, if the type of ptr is the
 * struct type.
 */
#define rt_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define rt_slist_entry(node, type, member) \
    rt_container_of(node, type, member)

/**
 * Single List structure
 */
struct rt_slist_node
{
    struct rt_slist_node *next;                         /**< point to next node. */
};
typedef struct rt_slist_node rt_slist_t;                /**< Type for single list. */

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
rt_inline void rt_slist_init(rt_slist_t *l)
{
    l->next = RT_NULL;
}

rt_inline void rt_slist_append(rt_slist_t *l, rt_slist_t *n)
{
    struct rt_slist_node *node;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = RT_NULL;
}

rt_inline int rt_slist_isempty(rt_slist_t *l)
{
    return l->next == RT_NULL;
}

rt_inline rt_slist_t *rt_slist_remove(rt_slist_t *l, rt_slist_t *n)
{
    /* remove slist head */
    struct rt_slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (rt_slist_t *)0) node->next = node->next->next;

    return l;
}

rt_inline rt_slist_t *rt_slist_next(rt_slist_t *n)
{
    return n->next;
}

rt_inline rt_slist_t *rt_slist_first(rt_slist_t *l)
{
    return l->next;
}

rt_inline void rt_slist_insert(rt_slist_t *l, rt_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}
/********************************************Slist type definitions end*********************************************/

/********************************************Device type definitions ***********************************************/
/**
 * Device structure
 */
struct uart_device
{
    char name[RT_NAME_MAX];                       /**< name of uart device */
};
typedef struct uart_device *rt_device_t;


/********************************************Device type definitions end********************************************/

/********************************************OS type definitions ***************************************************/
#define RT_THREAD_PRIORITY_MAX         (32)
#define RT_TICK_PER_SECOND	           (1000) //configTICK_RATE_HZ     /* Tick per Second*/
#define UART_RX_BUFF_SIZE_MAX 		   (5*1024)

typedef osSemaphoreId_t rt_sem_t;
typedef osMutexId_t rt_mutex_t;
typedef osThreadId_t rt_thread_t;
typedef osMessageQueueId_t osMsgQueId_t;
typedef osEventFlagsId_t rt_event_t;
typedef osTimerId_t osTimeId_t ;
#define rt_calloc calloc
#define rt_realloc realloc
#define rt_malloc malloc //pvPortMalloc
#define rt_free free     //vPortFree
#define rt_memcmp memcmp
#define rt_memset memset
#define rt_memcpy memcpy

#define rt_strstr strstr
#define rt_strlen strlen
#define rt_strcmp strcmp
#define rt_strncpy strncpy
#define rt_strncmp strncmp
#define rt_snprintf snprintf
#define rt_kprintf LOG_V

#define rt_tick_get xTaskGetTickCount
#define rt_thread_mdelay(ms) osDelay(rt_tick_from_millisecond(ms))

#define rt_mutex_create(name, flag) (osMutexNew(NULL))
#define rt_mutex_take osMutexAcquire
#define rt_mutex_release osMutexRelease
#define rt_mutex_delete osMutexDelete

#define rt_sem_create(name, value, flag) (osSemaphoreNew(1, 0, NULL))
#define rt_sem_take osSemaphoreAcquire
#define rt_sem_control(sem, cmd, arg) do{osSemaphoreDelete(sem); sem = osSemaphoreNew(1, 0, NULL);}while(0)
#define rt_sem_release osSemaphoreRelease
#define rt_sem_delete osSemaphoreDelete

#define os_sem_create(max_count, initial_count) osSemaphoreNew(max_count, initial_count, NULL)
#define os_sem_take osSemaphoreAcquire
#define os_sem_release osSemaphoreRelease
#define os_sem_delete osSemaphoreDelete

#define OsalMsgQCreate osMessageQueueNew
#define OsalMsgQDelete osMessageQueueDelete
#define OsalMsgRcv osMessageQueueGet
#define OsalMsgSend osMessageQueuePut
#define OsalMsgGetCount osMessageQueueGetCount
#define OsalMsgGetSpace osMessageQueueGetSpace

#define OsalEventNCreate osEventFlagsNew
#define OsalEventDelete osEventFlagsDelete
#define OsalEventSend osEventFlagsSet
#define OsalEventRecv osEventFlagsWait
#define OsalEventFlagsGet osEventFlagsGet

#define OsalTimerCreate osTimerNew
#define OsalTimerStart  osTimerStart
#define OsalTimerDelete osTimerDelete
#define OsalTimerStop osTimerStop

#define rt_thread_create(name,entry,parameter,size,priorit,tick) ({osThreadAttr_t attr = {.name = name, .stack_size = size, .priority = (osPriority_t)priorit}; osThreadId_t osThreadId = osThreadNew(entry, parameter, &attr); osThreadId;})
#define rt_thread_startup(parser)  
#define os_thread_create osThreadNew
#define os_thread_startup rt_thread_startup
#define os_thread_destroy osThreadTerminate
#define os_thread_get_stack_space osThreadGetStackSpace
#define os_thread_get_thread_id osThreadGetId
#define os_thread_exit osThreadExit 
#define os_thread_get_free_heap_size xPortGetFreeHeapSize 
#define os_thread_detach  osThreadDetach 
#define os_thread_get_min_free_heap_size xPortGetMinimumEverFreeHeapSize

#define rt_thread_delay osDelay

#define rt_device_find(objname) ({rt_device_t device = (rt_device_t)rt_calloc(1, sizeof(struct uart_device)); rt_strncpy((char *)device->name, (const char*)objname, RT_NAME_MAX); device;})

#define RT_ASSERT(EX)       ({if (!(EX)) {assert(0);}})
#define LWIP_ASSERT(message, assertion)
#define rt_hw_interrupt_disable() NULL
#define rt_hw_interrupt_enable(level) NULL

#define portMAX_DELAY  0xffffffffUL //HAL_MAX_DELAY

#define parameter_range_check(value, min, max, default)  ({if (((value) < (min)) || ((value) > (max))) {(value) = default;LOG_W("The parameter is out of bounds. Restore the default value");}})

#define ALIGN_UP(x, a) ( ( ((x) + ((a) - 1) ) / a ) * a )
#define ALIGN_DOWN(x, a) ( ( (x) / (a)) * (a) )
/********************************************OS type definitions end************************************************/

/********************************************function type definitions *********************************************/
rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflag);
rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
rt_err_t rt_device_set_rx_indicate(rt_device_t dev, rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size));
rt_err_t rt_device_close(rt_device_t dev);
rt_err_t rt_device_cb(void *buffer, rt_size_t size);
rt_tick_t rt_tick_from_millisecond(rt_int32_t ms);
u32_t ipaddr_addr(const char *cp);
/********************************************function type definitions end******************************************/
#endif /* __AT_OSAL_H__ */
