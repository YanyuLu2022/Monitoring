#include "cmsis_os.h"
#include "FreeRTOS.h"     // ARM.FreeRTOS::RTOS:Core
#include "task.h"         // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"       // ARM.FreeRTOS::RTOS:Core
#include "tim.h"

#include "MY_usart.h"

// 串口发送时是否要等待数据发送完成,如果启用的话不能再中断里面发送
#define Tx1Semaphore 0

#define Tx2Semaphore 0

#define Usart1_Thaerd_BufferLenght 100

// 信号量句柄

static SemaphoreHandle_t USART1_TxSemaphore; // 发送


static SemaphoreHandle_t USART2_TxSemaphore; // 发送
// 队列句柄
static QueueHandle_t Uart1_Queue = NULL;
static QueueHandle_t Uart2_Queue = NULL;
// CreateSemaphore
static Uart1_Struct Uart1Rx_Buffer; // 接收缓冲区
uint16_t Uart1Rx_Counter = 0;            // 下标
uint16_t Uart1Rx_Counter_Up = 0;         // 上一次下标
uint8_t Uart1Rx_char = {0};              // 接收字符

static Uart2_Struct Uart2Rx_Buffer; // 接收缓冲区
uint16_t Uart2Rx_Counter = 0;            // 下标
uint16_t Uart2Rx_Counter_Up = 0;         // 上一次下标
uint8_t Uart2Rx_char = {0};              // 接收字符
static uint16_t RTUartRx_Timer = 0;

BaseType_t xHigherPriorityTaskWoken = pdFALSE;



// 清除缓冲区
void Uart1Rx_Buffer_Close(void)
{
    Uart1Rx_Counter = 0;
    Uart1Rx_Counter_Up = 0;
    memset(Uart1Rx_Buffer.rx_str, 0, sizeof(Uart1Rx_Buffer.rx_str));
}

void Uart2Rx_Buffer_Close(void)
{
    Uart2Rx_Counter = 0;
    Uart2Rx_Counter_Up = 0;
    memset(Uart2Rx_Buffer.rx_str, 0, sizeof(Uart2Rx_Buffer.rx_str));
}

void GameSoundTimer_Func(void)
{

    if (Uart1_Queue != NULL)
    {
        if (Uart1Rx_Counter_Up == Uart1Rx_Counter && Uart1Rx_Counter != 0)
        {
            Uart1Rx_Buffer.num = Uart1Rx_Counter;
            xQueueSendToBackFromISR(Uart1_Queue, &Uart1Rx_Buffer, 0);
            Uart1Rx_Buffer_Close();
        }
        Uart1Rx_Counter_Up = Uart1Rx_Counter;
    }
    if (Uart2_Queue != NULL)
    {
        if (Uart2Rx_Counter_Up == Uart2Rx_Counter && Uart2Rx_Counter != 0)
        {
             Uart2Rx_Buffer.num = Uart2Rx_Counter;
            xQueueSendToBackFromISR(Uart2_Queue, &Uart2Rx_Buffer, 0);
            Uart2Rx_Buffer_Close();
        }
        Uart2Rx_Counter_Up = Uart2Rx_Counter;
    }
}

void My_Uart1_Init(void)
{
    Uart1_Queue = xQueueCreate(Uart1QueueLenght, sizeof(Uart1_Struct));
    USART1_TxSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(USART1_TxSemaphore);    
    HAL_UART_Receive_IT(&huart1, &Uart1Rx_char, 1);
    vTaskDelay(10);
    if (NULL == RTUartRx_Timer)
    {
        RTUartRx_Timer = 1;
		HAL_TIM_Base_Start_IT(&htim2);
    }
}
void My_Uart2_Init(void)
{
	
    Uart2_Queue = xQueueCreate(Uart2QueueLenght, sizeof(Uart2_Struct));
    USART2_TxSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(USART2_TxSemaphore);
    HAL_UART_Receive_IT(&huart2, &Uart2Rx_char, 1);    
    if (NULL == RTUartRx_Timer)
    {
        RTUartRx_Timer = 1;
		HAL_TIM_Base_Start_IT(&htim2);
    }
}
/**
返回Uart1_Queue句柄地址
*/
QueueHandle_t Return1_P_UartStruct(void)
{
    return Uart1_Queue;
}

/**
返回Uar2t_Queue句柄地址
*/
QueueHandle_t Return2_P_UartStruct(void)
{
    return Uart2_Queue;
}





static void Stm32_uart1_Send(uint8_t *datas, int len, int timeout_ms)
{


    // 等待信号量
    xSemaphoreTake(USART1_TxSemaphore, timeout_ms);
    // 串口发送数据
    HAL_UART_Transmit_IT(&huart1, datas, len);
}
static void Stm32_uart2_Send(uint8_t *datas, int len, int timeout_ms)
{

    // 等待信号量
    xSemaphoreTake(USART2_TxSemaphore, timeout_ms);
    // 串口发送数据
    HAL_UART_Transmit_IT(&huart2, datas, len);

}
void UsartPrintf(uint8_t usart, int time, char *fmt, ...)
{

    static unsigned char UsartPrintfBuf[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);
    va_end(ap);
    if(usart == 1)
    {
        Stm32_uart1_Send((uint8_t *)UsartPrintfBuf, sizeof(UsartPrintfBuf)/sizeof(UsartPrintfBuf[0]), time);
    }
    if(usart == 2)
    {
        Stm32_uart2_Send((uint8_t *)UsartPrintfBuf, sizeof(UsartPrintfBuf)/sizeof(UsartPrintfBuf[0]), time);
    }
}
void Usart_TX_Data(uint8_t usart, int time, char *fmt,uint8_t leght)
{

    if(usart == 1)
    {
        Stm32_uart1_Send((uint8_t *)fmt, leght, time);
    }
    if(usart == 2)
    {
        Stm32_uart2_Send((uint8_t *)fmt, leght, time);
    }
}








void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        // 释放信号量
        xSemaphoreGiveFromISR(USART1_TxSemaphore, NULL);
    }
    if (huart == &huart2)
    {
        // 释放信号量
        xSemaphoreGiveFromISR(USART2_TxSemaphore, NULL);
    }    
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) // 串口3接收完成回调函数
{
    if (huart->Instance == USART1)
    {

        Uart1Rx_Buffer.rx_str[Uart1Rx_Counter++] = Uart1Rx_char;
        if (Uart1Rx_Counter >= RX1_Counter_Max)
        {
            Uart1Rx_Buffer_Close();
        }
        Uart1Rx_char = 0;
        HAL_UART_Receive_IT(&huart1, &Uart1Rx_char, 1); // 重新使能中断
    }
    if (huart->Instance == USART2)
    {

        Uart2Rx_Buffer.rx_str[Uart2Rx_Counter++] = Uart2Rx_char;
        if (Uart2Rx_Counter >= RX2_Counter_Max)
        {
            Uart2Rx_Buffer_Close();
        }
        Uart2Rx_char = 0;
        HAL_UART_Receive_IT(&huart2, &Uart2Rx_char, 1); // 重新使能中断
    }
}

