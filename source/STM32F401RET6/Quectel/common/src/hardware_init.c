#include "main.h"
#include "at_osal.h"
#include "user_main.h"
#include "sd_fatfs.h"
#include "cmsis_os.h"
//#include "fatfs.h"

extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart2;
#define UAER_RECE_PRINT 0

#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART2 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

// GETCHAR_PROTOTYPE
// {
//   uint8_t ch = 0;
//   __HAL_UART_CLEAR_OREFLAG(&huart6);
//   HAL_UART_Receive(&huart6, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
//   HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, 0);
//   return ch;
// }

uint8_t g_uart_buf[256];
uint8_t g_uart_debug_buf[256];
uint16_t g_uart_debug_buf_get_len = 0;

void USER_GET_DEBUG_INPUT_DATA(const uint8_t **pData, uint16_t *pSize)
{
  *pData = g_uart_debug_buf;
  *pSize = g_uart_debug_buf_get_len;
}
//  printf("%d = %d  =%d \r\n", num, huart->RxXferCount,huart->RxXferSize);
void USER_UART6_RxIdleCallback(UART_HandleTypeDef *huart)
{
  static uint16_t  RxXferCountOld = sizeof(g_uart_debug_buf)/sizeof(g_uart_debug_buf[0]);
  int ret = 0, i, num = 0;
  char back_space[3] = {0x8, 0x20, 0x8};

  num = RxXferCountOld-huart->RxXferCount;
  if ((num == 1) && (g_uart_debug_buf[huart->RxXferSize-huart->RxXferCount-num] == 0x8))   //back space
  {
    if (huart->RxXferCount == huart->RxXferSize-1)
    {
      huart->pRxBuffPtr-=1;
      huart->RxXferCount+=1;
    }
    else
    {
      HAL_UART_Transmit(&huart6, back_space,3,1000);
      huart->pRxBuffPtr-=2;
      huart->RxXferCount+=2;
    }
  }
  else
    HAL_UART_Transmit(&huart6, &g_uart_debug_buf[huart->RxXferSize-huart->RxXferCount-num],num,1000);
  //printf("whz %u, %u, 0x%x, 0x%x, %d, %d, %d\r\n", huart->RxXferCount, huart->RxXferSize, g_uart_debug_buf[0], g_uart_debug_buf[1], huart->RxState, RxXferCountOld, num);
  RxXferCountOld = huart->RxXferCount;
    // for (i=0; i<huart->RxXferSize-huart->RxXferCount; i++)
    //  printf("%d = 0x%x\r\n", i, g_uart_debug_buf[i]);
    // printf("g_uart_debug_buf = 0x%x\r\n",g_uart_debug_buf[ huart->RxXferSize-huart->RxXferCount-1]);
    // printf("g_uart_debug_buf = 0x%x\r\n", g_uart_debug_buf[ huart->RxXferSize-huart->RxXferCount-2]);
    // printf("len = %d\r\n", huart->RxXferSize-huart->RxXferCount);
  if ((huart->RxXferSize - huart->RxXferCount) >= 1&&(g_uart_debug_buf[huart->RxXferSize-huart->RxXferCount-1] == 0x0d))
  {
	 // printf("g_uart_debug_buf = 0x%x\r\n", huart->RxXferSize-huart->RxXferCount-1);
  RxXferCountOld = sizeof(g_uart_debug_buf)/sizeof(g_uart_debug_buf[0]);
  huart->RxState = HAL_UART_STATE_READY;
  g_uart_debug_buf_get_len = huart->RxXferSize-huart->RxXferCount - 1;
    serial_input_parse_thread_wake_up();
  }
  else if ((huart->RxXferSize - huart->RxXferCount) >= 2 && (g_uart_debug_buf[huart->RxXferSize - huart->RxXferCount - 2] == 0x0d))
  {
  RxXferCountOld = sizeof(g_uart_debug_buf)/sizeof(g_uart_debug_buf[0]);
  huart->RxState = HAL_UART_STATE_READY;
  g_uart_debug_buf_get_len = huart->RxXferSize-huart->RxXferCount - 2;
    serial_input_parse_thread_wake_up();
  }
      
  

  ret = HAL_UART_Receive_IT(huart, (uint8_t *)g_uart_debug_buf, sizeof(g_uart_debug_buf)/sizeof(g_uart_debug_buf[0]));
}

rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL);
    HAL_UART_Transmit(&huart2, buffer, size, 0xFFFF);
    return size;
}

//https://blog.csdn.net/xuanjianqiang/article/details/123083526
static uint32_t g_head_ptr = 0;
void USER_UART2_RxIdleCallback(UART_HandleTypeDef *huart)
{
    uint32_t tail_ptr;
    uint32_t copy, offset;
    
    /*
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     * |     head_ptr          tail_ptr         |
     * |         |                 |            |
     * |         v                 v            |
     * | --------*******************----------- |
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     */
    
    /* Already received */
    tail_ptr = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    
    offset = g_head_ptr % huart->RxXferSize;
    copy = tail_ptr - offset;
    g_head_ptr += copy;
    
    rt_device_cb(huart->pRxBuffPtr + offset, copy);
}

void USER_UART2_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t tail_ptr;
    uint32_t offset, copy;
    
    /*
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     * |                  half                  |
     * |     head_ptr   tail_ptr                |
     * |         |          |                   |
     * |         v          v                   |
     * | --------*******************----------- |
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     */
    
    tail_ptr = (huart->RxXferSize >> 1) + (huart->RxXferSize & 1);
    
    offset = g_head_ptr % huart->RxXferSize;
    copy = tail_ptr - offset;
    g_head_ptr += copy;
    
    rt_device_cb(huart->pRxBuffPtr + offset, copy);
}

void USER_UART2_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t tail_ptr;
    uint32_t offset, copy;
    
    /*
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     * |                  half                  |
     * |                    | head_ptr tail_ptr |
     * |                    |    |            | |
     * |                    v    v            v |
     * | ------------------------************** |
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     */

    tail_ptr = huart->RxXferSize;
    
    offset = g_head_ptr % huart->RxXferSize;
    copy = tail_ptr - offset;
    g_head_ptr += copy;
    
    rt_device_cb(huart->pRxBuffPtr + offset, copy);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    /* Ignore half-full interrupts when receiving a buf size of 1 */
    if(1 == huart->RxXferSize) { return ; }
    
    if((huart->Instance) == USART2)
    {
        USER_UART2_RxHalfCpltCallback(huart);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if((huart->Instance) == USART2)
    {
      if(NULL == huart->hdmarx)
      {
          /* If in interrupt mode, restore the receive address pointer to the initialized buffer position */
          huart->pRxBuffPtr -= huart->RxXferSize;
      }
    
        USER_UART2_RxCpltCallback(huart);

        if(NULL != huart->hdmarx)
        {
            if(huart->hdmarx->Init.Mode != DMA_CIRCULAR)
            {
                while(HAL_OK != HAL_UART_Receive_DMA(huart, huart->pRxBuffPtr, huart->RxXferSize))
                {
                    __HAL_UNLOCK(huart);
                }
            }
        }
        else
        {
            while(HAL_UART_Receive_IT(huart, huart->pRxBuffPtr, huart->RxXferSize))
            {
                __HAL_UNLOCK(huart);
            }
        }
    }
}

/**
  * @brief  Reception Event Callback (Rx event notification called after use of advanced reception service).
  * @param  huart UART handle
  * @param  Size  Number of data available in application reception buffer (indicates a position in
  *               reception buffer until which, data are available)
  * @retval None
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) 
{
  if(huart->Instance == USART2)
  {  
    // HAL_UART_DMAStop(&huart2);//DMA stop to reset how much data to send, avoid data errors
    // HAL_UARTEx_ReceiveToIdle_DMA(&huart2,(uint8_t *)g_uart_buf, sizeof(g_uart_buf)/sizeof(g_uart_buf[0])));//Continue to enable idle interrupt DMA sending
  }
}

/**
  * @brief  UART error callbacks.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  __IO uint32_t tmpErr = 0x00U;

  tmpErr = HAL_UART_GetError(huart);
  if(HAL_UART_ERROR_NONE == tmpErr)
  {
    return ;
  }

  switch(tmpErr)
  {
    case HAL_UART_ERROR_PE:
      __HAL_UART_CLEAR_PEFLAG(huart);
      break;
    case HAL_UART_ERROR_NE:
      __HAL_UART_CLEAR_NEFLAG(huart);
      break;
    case HAL_UART_ERROR_FE:
      __HAL_UART_CLEAR_FEFLAG(huart);
      break;
    case HAL_UART_ERROR_ORE:
      __HAL_UART_CLEAR_OREFLAG(huart);
      break;
    case HAL_UART_ERROR_DMA:

      break;
    default:
      break;
  }

  if(NULL != huart->hdmarx)
  {
    while(HAL_UART_Receive_DMA(huart, huart->pRxBuffPtr, huart->RxXferSize))
    {
      __HAL_UNLOCK(huart);
    }
  }
  else
  {
    /* Restore the receiving address pointer to the initial buffer position, initial address = current address - Number of data received, number of data received = Number of data to be received - number of not received*/
    while(HAL_UART_Receive_IT(huart, huart->pRxBuffPtr - (huart->RxXferSize - huart->RxXferCount), huart->RxXferSize))
    {
      __HAL_UNLOCK(huart);
    }
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  //LOG_V("GPIO_Pin %d callback, level is %d", GPIO_Pin, HAL_GPIO_ReadPin(GPIOA, GPIO_Pin));
}

void hardware_init(void)
{
    //Idle interrupt huart2
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);       // Enable UART2 IDLE interrupt
    HAL_UART_Receive_DMA(&huart2, (uint8_t *)g_uart_buf, sizeof(g_uart_buf)/sizeof(g_uart_buf[0]));  //Enable DMA reception

    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);       // Enable UART6 IDLE interrupt
    HAL_UART_Receive_IT(&huart6, (uint8_t *)g_uart_debug_buf, sizeof(g_uart_debug_buf)/sizeof(g_uart_debug_buf[0]));

    HAL_UART_Receive_DMA(&huart2, (uint8_t *)g_uart_buf, sizeof(g_uart_buf)/sizeof(g_uart_buf[0]));//Enable idle serial port to interrupt DMA receiving data
    //setvbuf(stdin, NULL, _IONBF, 0);

}
