#include "stdio.h"
#include "string.h"
#include <stdarg.h>
#include <stdlib.h>

#include "My_Voice.h"
#include "My_Usart.h"

#define MP3_Thread_lenght 128 // 任务堆栈大小

static QueueHandle_t MP3_Queue = NULL;            // 队列句柄
static uint8_t vol = 0x08;                        // 声音
static uint8_t mus = 0x01;                        // 音乐
static uint8_t vce = 0x01;                        // 提示
StackType_t MP3_Thread_Buffer[MP3_Thread_lenght]; // 任务存储数组

QueueHandle_t ReMP3_QueueStruct(void) // 返回队列句柄
{
    return MP3_Queue;
}
void Voice_Init(void)
{
    MP3_Queue = xQueueCreate(MP3QueueLenght, sizeof(MP3_Queue)); // 创建队列
}
uint8_t Voice_Get(uint8_t Svce)
{
    if (Svce != 0)
        vce = Svce;
    return vce;
}

uint8_t Music_Get(uint8_t Smus)
{
    if (Smus == 0)
    {
        return mus;
    }
    else if (Smus == 1)
    {
        mus = 0X01;
        return mus;
    }
    else if (Smus == 2)
    {
        mus += 1;
        if (mus > 0x07)
        {
            mus = 0x01;
        }
        return mus;
    }
    else if (Smus == 3)
    {
        mus -= 1;
        if (mus < 0x01)
        {
            mus = 0x07;
        }
        return mus;
    }
}

uint8_t Volume_Get(uint8_t Svol)
{
    if (Svol == 0)
    {
        return vol;
    }
    else if (Svol == 1)
    {
        vol -= 2;
        if (vol < 0x00)
        {
            vol = 0x00;
        }
        return vol;
    }
    else if (Svol == 2)
    {
        vol += 2;
        if (vol > 0x1E)
        {
            vol = 0x1E;
        }
        return vol;
    }
}
void MP3_Thread(void *argument)
{
    My_Uart1_Init();
    static MP3_Struct MP3_Data = {0};
    static char tx_str[8] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x08, 0xEF}; // 默认音量

    vTaskDelay(200);
    Usart_TX_Data(1, 1000, tx_str, 8); // 调节音量
    while (1)
    {

        xQueueReceive(MP3_Queue, &MP3_Data, portMAX_DELAY); // 队列接收数据
        if (MP3_Data.volume != 0)
        {
            tx_str[2] = 0x06;
            tx_str[3] = 0x06;
            tx_str[4] = 0x00;
            tx_str[5] = 0x00;
            tx_str[6] = Volume_Get(MP3_Data.volume);
            Usart_TX_Data(1, 1000, tx_str, 8); // 队列发送数据
            vTaskDelay(10);                    // 任务延时
        }
        if (MP3_Data.music != 0)
        {
            if (MP3_Data.music == 4)
            {
                tx_str[2] = 0x06;
                tx_str[3] = 0x16;
                tx_str[4] = 0x00;
                tx_str[5] = 0x00;
                tx_str[6] = 0x00;
            }
            else
            {
                tx_str[2] = 0x06;
                tx_str[3] = 0x12;
                tx_str[4] = 0x00;
                tx_str[5] = 0x00;
                tx_str[6] = Music_Get(MP3_Data.music);
            }
            Usart_TX_Data(1, 1000, tx_str, 8); // 队列发送数据
            vTaskDelay(10);                    // 任务延时
        }
        if (MP3_Data.voice != 0)
        {
            tx_str[2] = 0x06;
            tx_str[3] = 0x14;
            tx_str[4] = 0x00;
            tx_str[5] = 0x02;
            tx_str[6] = Voice_Get(MP3_Data.voice);
            Usart_TX_Data(1, 1000, tx_str, 8); // 队列发送数据
            vTaskDelay(10);                    // 任务延时
        }
    }

} // 任务

void MP3_Thread_Start(StaticTask_t *PThread, void *Parameters)
{
    uint32_t MP3_Stack_L = sizeof(MP3_Thread_Buffer[MP3_Thread_lenght] / MP3_Thread_Buffer[0]);
    xTaskCreateStatic(MP3_Thread,
                      "MP3",
                      MP3_Stack_L,
                      Parameters,
                      (osPriority_t)osPriorityNormal,
                      MP3_Thread_Buffer,
                      PThread);
                      
}
