
#ifndef __RINGBUFFER_h__
#define __RINGBUFFER_h__

#include <stdint.h>
#include <string.h>

struct ringbuffer
{
  uint8_t*  buffer;
  uint16_t  buffer_size;
  volatile uint16_t  head; 
  volatile uint16_t  tail; 
};

int ringbuffer_init(struct ringbuffer* rb, uint8_t* buffer, uint16_t size );
uint16_t ringbuffer_data_len(struct ringbuffer* rb );
uint16_t ringbuffer_putstr(struct ringbuffer* rb, const uint8_t* data, uint16_t data_length);
int ringbuffer_getstr(struct ringbuffer* rb, uint8_t* data, uint16_t data_length);

// void rb_data_putchar(const uint8_t ch);
// uint16_t get_uart_recv_sta(void);
// void uart_recv_sta_clean(void);
// void set_uart_recv_sta(void);

#endif // __RINGBUFFER_h__
