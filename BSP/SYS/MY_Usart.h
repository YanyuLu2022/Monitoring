#ifndef __MY_Usart_H
#define __MY_Usart_H
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "FreeRTOS.h"  
#include "queue.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include <stdarg.h>
#include <stdlib.h>
//队列长度
#define Uart1QueueLenght 10
#define Uart2QueueLenght 10
#define RX1_Counter_Max 12
#define RX2_Counter_Max 12
typedef struct 
{
    uint8_t rx_str[RX1_Counter_Max];
    int num;
}Uart1_Struct;
typedef struct 
{
    uint8_t rx_str[RX2_Counter_Max];
    int num;
}Uart2_Struct;

void My_Uart1_Init(void);
void My_Uart2_Init(void);
QueueHandle_t Return1_P_UartStruct(void);
QueueHandle_t Return2_P_UartStruct(void);
void UsartPrintf(uint8_t usart, int time, char *fmt, ...);

void Usart_TX_Data(uint8_t usart, int time, char *fmt,uint8_t leght);
#endif // __MY_Usart_H

