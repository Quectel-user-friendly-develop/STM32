/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-01     armink       first version
 * 2018-04-04     chenyong     add base commands
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "ringbuffer.h"
#include "at_osal.h"

struct ringbuffer g_uart_rxcb = {.buffer = NULL, .buffer_size = 0, .head = 0, .tail = 0};
static uint8_t g_uart_rx_buf[UART_RX_BUFF_SIZE_MAX] = {0};  

typedef rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size);
static rx_ind g_rx_ind_cfun = NULL;
static rt_device_t g_dev = NULL;

/**
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 *
 * @param cp IP address in ascii representation (e.g. "127.0.0.1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int osal_ip4addr_aton(const char *cp, ip4_addr_t *addr)
{
    uint32_t val;
    uint8_t base;
    char c;
    uint32_t parts[4];
    uint32_t *pp = parts;

    c = *cp;
    for (;;)
    {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, 1-9=decimal.
         */
        if (!isdigit(c))
        {
            return 0;
        }
        val = 0;
        base = 10;
        if (c == '0')
        {
            c = *++cp;
            if (c == 'x' || c == 'X')
            {
                base = 16;
                c = *++cp;
            }
            else
            {
                base = 8;
            }
        }
        for (;;)
        {
            if (isdigit(c))
            {
                val = (val * base) + (uint32_t) (c - '0');
                c = *++cp;
            }
            else if (base == 16 && isxdigit(c))
            {
                val = (val << 4) | (uint32_t) (c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            }
            else
            {
                break;
            }
        }
        if (c == '.')
        {
            /*
             * Internet format:
             *  a.b.c.d
             *  a.b.c   (with c treated as 16 bits)
             *  a.b (with b treated as 24 bits)
             */
            if (pp >= parts + 3)
            {
                return 0;
            }
            *pp++ = val;
            c = *++cp;
        }
        else
        {
            break;
        }
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && !isspace(c))
    {
        return 0;
    }
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    switch (pp - parts + 1)
    {

    case 0:
        return 0; /* initial nondigit */

    case 1: /* a -- 32 bits */
        break;

    case 2: /* a.b -- 8.24 bits */
        if (val > 0xffffffUL)
        {
            return 0;
        }
        if (parts[0] > 0xff)
        {
            return 0;
        }
        val |= parts[0] << 24;
        break;

    case 3: /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff)
        {
            return 0;
        }
        if ((parts[0] > 0xff) || (parts[1] > 0xff))
        {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4: /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff)
        {
            return 0;
        }
        if ((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff))
        {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    default:
        RT_ASSERT(0);
        break;
    }
    if (addr)
    {
        ip4_addr_set_u32(addr, htonl(val));
    }
    return 1;
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *ipaddr_ntoa_r(const ip_addr_t *addr, char *buf, int buflen)
{
  u32_t s_addr;
  char inv[3];
  char *rp;
  u8_t *ap;
  u8_t rem;
  u8_t n;
  u8_t i;
  int len = 0;

  s_addr = ip4_addr_get_u32(addr);

  rp = buf;
  ap = (u8_t *)&s_addr;
  for(n = 0; n < 4; n++) {
    i = 0;
    do {
      rem = *ap % (u8_t)10;
      *ap /= (u8_t)10;
      inv[i++] = '0' + rem;
    } while(*ap);
    while(i--) {
      if (len++ >= buflen) {
        return NULL;
      }
      *rp++ = inv[i];
    }
    if (len++ >= buflen) {
      return NULL;
    }
    *rp++ = '.';
    ap++;
  }
  *--rp = 0;
  return buf;
}

/**
 * Convert numeric IP address into decimal dotted ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         represenation of addr
 */
char *
ipaddr_ntoa(const ip_addr_t *addr)
{
  static char str[16];
  return ipaddr_ntoa_r(addr, str, 16);
}

/**
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @return ip address in network order
 */
u32_t ipaddr_addr(const char *cp)
{
  ip_addr_t val;

  if (inet_aton(cp, &val)) {
    return ip4_addr_get_u32(&val);
  }
  return (IPADDR_NONE);
}


rt_tick_t rt_tick_from_millisecond(rt_int32_t ms)
{
    rt_tick_t tick;

    if (ms < 0)
    {
        tick = (rt_tick_t)RT_WAITING_FOREVER;
    }
    else
    {
        tick = RT_TICK_PER_SECOND * (ms / 1000);
        tick += (RT_TICK_PER_SECOND * (ms % 1000) + 999) / 1000;
    }
    
    /* return the calculated tick */
    return tick;
};

rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflag)
{
    RT_ASSERT(dev != RT_NULL);
    if (g_uart_rxcb.buffer == NULL)
        ringbuffer_init(&g_uart_rxcb, g_uart_rx_buf, UART_RX_BUFF_SIZE_MAX);

    return RT_EOK;
}

// __weak rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
// {
//     RT_ASSERT(dev != RT_NULL);
//     HAL_UART_Transmit(&huart2, buffer, size, 0xFFFF);
//     return RT_EOK;
// }

rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    // RT_ASSERT(dev != RT_NULL);
    // RT_ASSERT(buffer != RT_NULL);

    if (g_uart_rxcb.buffer == NULL)
        ringbuffer_init(&g_uart_rxcb, g_uart_rx_buf, UART_RX_BUFF_SIZE_MAX);

    if (ringbuffer_data_len(&g_uart_rxcb) > 0)
    {
        ringbuffer_getstr(&g_uart_rxcb, (uint8_t* )buffer, size);
        return size;
    }
    
    return 0;
}

rt_err_t rt_device_set_rx_indicate(rt_device_t dev, rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size))
{
    RT_ASSERT(dev != RT_NULL);
    RT_ASSERT(rx_ind != RT_NULL);

    g_rx_ind_cfun = rx_ind;
    g_dev = dev;
    
    return RT_EOK;
}

rt_err_t rt_device_cb(void *buffer, rt_size_t size)
{
    ringbuffer_putstr(&g_uart_rxcb,buffer,size);
    if (g_rx_ind_cfun)
        g_rx_ind_cfun(g_dev, size);
    return RT_EOK;
}

rt_err_t rt_device_close(rt_device_t dev)
{
    RT_ASSERT(dev != RT_NULL);
    return RT_EOK;
}
