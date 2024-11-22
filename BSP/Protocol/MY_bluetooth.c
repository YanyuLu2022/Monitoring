#include "stdio.h"
#include "string.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"     // ARM.FreeRTOS::RTOS:Core
#include "task.h"         // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"       // ARM.FreeRTOS::RTOS:Core
#include <stdarg.h>
#include <stdlib.h>

#include "MY_bluetooth.h"

// 串口发送时是否要等待数据发送完成,如果启用的话不能再中断里面发送
#define TxSemaphore 0
#define RX_Counter_Max 4

// 信号量句柄
static SemaphoreHandle_t USART1_RxSemaphore; // 接收
static SemaphoreHandle_t USART1_TxSemaphore; // 发送
// 队列句柄
static QueueHandle_t Uart_Queue;
// CreateSemaphore
uint8_t Rx_Buffer[RX_Counter_Max]; // 接收缓冲区
uint8_t Rx_str[RX_Counter_Max];    // 接收到的数据
uint16_t Rx_Counter = 0;           // 下标
uint8_t Rx_char = {0};             // 接收字符
int Rx_state = 0;                  // 接收状态机的状态
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

static Uart_Struct Rx_Date = {0}; // 速度更新结构体

/**
返回Uart_Queue句柄地址
*/
QueueHandle_t Return_P_UartStruct(void)
{

    return Uart_Queue;
}

// 清除缓冲区
void Rx_Buffer_Close(void)
{
    Rx_Counter = 0;
    memset(Rx_Buffer, 0, sizeof(Rx_Buffer));
}
// 清空接收字符串
void Rx_Str_Close(void)
{
    memset(Rx_str, 0, sizeof(Rx_str));
}

// 发送函数
/**
void * xhusart 发送的串口
uint8_t *datas 发送的字符串
int len 字符串长度
int timeout_ms 等待时间
*/
static int Stm32_uart_Send(void *xhusart, uint8_t *datas, int len, int timeout_ms)
{

    // 串口发送数据
    HAL_UART_Transmit_IT(xhusart, datas, len);

    // 等待信号量
#if TxSemaphore
    if (pdTRUE != xSemaphoreTake(USART1_TxSemaphore, timeout_ms))
        return -1; // 信号量句柄
#endif
    return 0;
}

void UsartPrintf(void *pusart, int time, char *fmt, ...)
{

    static unsigned char UsartPrintfBuf[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);
    va_end(ap);
    Stm32_uart_Send(pusart, (uint8_t *)UsartPrintfBuf, sizeof(UsartPrintfBuf)/sizeof(UsartPrintfBuf[0]), time);
}

/*
    串口初始化
*/
void USART_Init(void)
{
	    // 创建二值信号量
    USART1_RxSemaphore = xSemaphoreCreateBinary();
    if (NULL == USART1_RxSemaphore)
        return;
    USART1_TxSemaphore = xSemaphoreCreateBinary();
    if (NULL == USART1_TxSemaphore)
        return;
    // 创建队列
    Uart_Queue = xQueueCreate(UartQueueLenght, sizeof(Uart_Struct));
}

void Usart1_Thread(void *argument)
{

    xSemaphoreTake(USART1_RxSemaphore, 0);
    HAL_UART_Receive_IT(&huart1, &Rx_char, 1);
    Rx_Str_Close();
    while (1)
    {
        xSemaphoreTake(USART1_RxSemaphore, portMAX_DELAY);
        Rx_Date.Uart_D = Rx_str[1];
        xQueueSend(Uart_Queue, &Rx_Date, 0);
        Rx_Str_Close();
    }
}
uint8_t Usart_Thread_Start(StaticTask_t *PThread, StackType_t *P_Stack, int L_Stack)
{
    if (NULL != xTaskCreateStatic(Usart1_Thread,
                                  "uart1_th",
                                  L_Stack,
                                  NULL,
                                  (osPriority_t)osPriorityNormal,
                                  P_Stack,
                                  PThread))
    {

        return 1;
    }
    return 0;
}

void Usart_Close(void *PThread)
{
    vTaskDelete(PThread); // 删除任务

    // vQueueDelete(Uart_Queue);
}

// 发送中断
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        // 释放信号量
        xSemaphoreGiveFromISR(USART1_TxSemaphore, NULL);
    }
}
/*
串口接收中断回调函数
*/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) // 串口3接收完成回调函数
{
    if (huart->Instance == USART1)
    {

        switch (Rx_state)
        {
        case 0:
            if (Rx_char == 0x25)
            {
                Rx_state = 1;
                Rx_Buffer[Rx_Counter++] = Rx_char;
            }
            else
            {
                Rx_Buffer_Close();
                Rx_state = 0;
            }
            break;
        case 1:
            if (Rx_Counter < RX_Counter_Max - 2)
            {
                Rx_Buffer[Rx_Counter++] = Rx_char;
            }
            else if (Rx_char == 0xA0)
            {
                Rx_Buffer[Rx_Counter++] = Rx_char;
                Rx_state = 2;
            }
            else
            {
                Rx_Buffer_Close();
                Rx_state = 0;
            }
            break;
        case 2:
            if (Rx_char == 0x55)
            {
                Rx_state = 0;
                Rx_Buffer[Rx_Counter] = Rx_char;
                memcpy(Rx_str, Rx_Buffer, sizeof(Rx_Buffer));
                Rx_Buffer_Close();
                xSemaphoreGiveFromISR(USART1_RxSemaphore, NULL);
            }
            else
            {
                Rx_state = 0;
                Rx_Buffer_Close();
            }
            break;
        }
        Rx_char = 0;
        HAL_UART_Receive_IT(&huart1, &Rx_char, 1); // 重新使能中断
    }
}
