#ifndef MY_Voice_H
#define MY_Voice_H
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"     // ARM.FreeRTOS::RTOS:Core
#include "task.h"         // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"       // ARM.FreeRTOS::RTOS:Core
#include "queue.h"
#include "usart.h"
//队列长度
#define MP3QueueLenght 10


#define MP3_Counter_Max 5
/**
 * volume : 
 * 1小声 
 * 2大声 
 * 0不变
 * music :
 * 0不变
 * 1播放音乐
 * 2上一首
 * 3下一首
 * 4停止
 * voice :
 * 0不变
 * 1：收到
 * 2：警报
 * 3：你干嘛
 * 
 */
typedef struct
{
    uint8_t volume;
	uint8_t music;
	uint8_t voice;
	
}MP3_Struct;//队列结构体
void MP3_Thread_Start(StaticTask_t *PThread, void *Parameters);
void Voice_Init(void);
QueueHandle_t ReMP3_QueueStruct(void); // 返回队列句柄
#endif // MY_Voice_H

