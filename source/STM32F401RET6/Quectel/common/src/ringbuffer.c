/****************************************************************************
*
* Copy right: 2020-, Copyrigths of Quectel Ltd.
****************************************************************************
* File name: ringbuffer.c
* History: Rev1.0 2020-12-1
****************************************************************************/

#include "ringbuffer.h"
#include "debug_service.h"
#include "at.h"
#include "cmsis_os.h"

#define ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
#define GET_MIN(x,y)  ((x) < (y) ? (x) : (y))
#define ringbuffer_space_len(rb) ((rb)->buffer_size - ringbuffer_data_len(rb))

// 最高位为串口接收完成标记位,低14位为当前接收数据长度
// volatile uint16_t g_uart_recv_sta = 0;	

// /**
//  * @description: 标记接收完成
//  * @param  none
//  * @return none
//  */
// void set_uart_recv_sta(void)
// {
// 	g_uart_recv_sta |= 1<<15;
// }

// /**
//  * @description: 返回单次数据接收长度
//  * @param  none
//  * @return none
//  */
// uint16_t get_uart_recv_data_len(void)
// {
// 	return g_uart_recv_sta; 		
// }

// /**
//  * @description: 查询标记接收状态位
//  * @param  none
//  * @return none
//  */
// uint16_t get_uart_recv_sta(void)
// {
// 	return (g_uart_recv_sta&(1<<15));
// }

// /**
//  * @description: 接收数据长度增加
//  * @param  none
//  * @return none
//  */
// void set_uart_recv_data_len(void)
// {
// 	g_uart_recv_sta ++;	
// }

// /**
//  * @description: 清除标记
//  * @param  none
//  * @return none
//  */
// void uart_recv_sta_clean(void)
// {
// 	g_uart_recv_sta = 0;	
// }

// /**
//  * @description: 开启定时器计数(弱函数,需用户实现) 
//  * @param  none
//  * @return none
//  */
// __weak void tim_start_count(void)
// {
// 	/*注意：当需要回调时,不应修改此函数,tim_start_count() 需要在用户文件中实现*/
// 	/* 该函数用于启动TIM定时器 */
// 	LOG_E("please implement this function< tim_start_count() > in the user program!");
// }

// /**
//  * @description: 清除定时器计数(弱函数,需用户实现) 
//  * @param  none
//  * @return none
//  */
// __weak void tim_clear_count(void)
// {
// 	/*注意：当需要回调时,不应修改此函数,tim_clear_count() 需要在用户文件中实现*/
// 	/* 该函数用于将TIM定时器计数清零 */
// 	LOG_E("please implement this function< tim_clear_count() > in the user program!");
// }

// /**
// * @description:  发送数据到rb
//  * @param  ch 待存储数据
//  * @return none
//  */
// void rb_data_putchar(const uint8_t ch)
// {
// 	if( 0 == get_uart_recv_sta() ) 						// 接收未完成
// 	{
// 		if(get_uart_recv_data_len() < BUFF_SIZE_MAX)
// 		{
// 			tim_clear_count();
// 			if( 0 == get_uart_recv_data_len() )
// 			{
// 				tim_start_count();						// 使能定时器7的中断
// 			}
// 			ringbuffer_putstr(&g_uart_rxcb,&ch,1);
// 			// 接收到一字节数据,进行长度累加
// 			set_uart_recv_data_len();
// 		}
// 		else
// 		{
// 			set_uart_recv_sta();
// 		}
// 	}
// }

/**
 * @description:  初始化rb
 * @param 			rb  		ringbuffer
 *					bf_pool 	内存池
 *					size		数据大小
 * @return 执行结果码
 */
int ringbuffer_init(struct ringbuffer* rb, uint8_t* bf_pool, uint16_t size)
{
    if (rb&&bf_pool)
    {
        rb->buffer = (uint8_t*)bf_pool;
        rb->buffer_size   = ALIGN_DOWN(size,4);
        rb->head   = 0;
        rb->tail   = 0;
        return 0;
    }
    else
        return -1;
}

/**
 * @description:  使用的rb大小的值
 * @param 			rb  ringbuffer
 * @return 执行结果码
 */
uint16_t ringbuffer_data_len(struct ringbuffer* rb)
{
    return ((rb->buffer_size - rb->head + rb->tail) % rb->buffer_size);
}

/**
 * @description: 发送数据到rb中
 * @param 			rb  			ringbuffer
 * 					data			待发送数据buffer
 * 					data_length		发送的数据长度
 * @return 执行结果码
 */
uint16_t ringbuffer_putstr(struct ringbuffer* rb, const uint8_t* data, uint16_t data_length )
{
    uint16_t space_len = rb->buffer_size - 1 - rb->tail;

    uint16_t put_data_len = GET_MIN(data_length, (space_len + rb->head) % rb->buffer_size);

    space_len++;

    memcpy(&rb->buffer[rb->tail], data, GET_MIN(put_data_len, space_len));

    if ( space_len < put_data_len )
    {
        memcpy( &rb->buffer[ 0 ], data + space_len, put_data_len - space_len );
    }

    rb->tail = (rb->tail + put_data_len) % rb->buffer_size;

    return put_data_len;
}

/**
 * @description: 从rb中读取数据
 * @param 			rb  			ringbuffer
 * 					data			待读取buffer
 * 					data_length		读取的数据长度
 * @return 执行结果码
 */
int ringbuffer_getstr(struct ringbuffer* rb, uint8_t* data, uint16_t data_length)
{
    uint16_t i, used_space, max_read_len, head;

    head = rb->head;

    used_space = ringbuffer_data_len(rb);

    max_read_len = GET_MIN(data_length, used_space);

    if ( max_read_len != 0)
    {
        for ( i = 0; i != max_read_len; i++, ( head = ( head + 1 ) % rb->buffer_size ) )
        {
            data[ i ] = rb->buffer[ head ];
        }
		rb->head = (rb->head + max_read_len) % rb->buffer_size;
    }
    return 0;
}
