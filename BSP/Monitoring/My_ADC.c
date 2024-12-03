#include "my_adc.h"
#include "cmsis_os.h"
#include <string.h>


// 队列句柄
static QueueHandle_t READC_Queue;

uint16_t Read_ADCData[2] = {0};
void My_ADC_Init(void)
{


  HAL_ADCEx_Calibration_Start(&hadc1);    //AD校准
  READC_Queue = xQueueCreate(READCQueueLenght, sizeof(ADCx_Queue_Data));

}
QueueHandle_t ReADC_QueueStruct(void)
{
	return READC_Queue;
}
void ADC_GETDATA_Thread(void *argument)
{
    ADCx_Queue_Data Get_ADCData;
	while (1)
	{

        ADC_ChannelConfTypeDef _adc;
        _adc.Channel=ADC_CHANNEL_7;
        _adc.Rank=1;
        _adc.SamplingTime=ADC_SAMPLETIME_239CYCLES_5;
        HAL_ADC_ConfigChannel(&hadc1,&_adc);
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1,10);
        vTaskDelay(5);
        Get_ADCData.air	= (float)(HAL_ADC_GetValue(&hadc1) *100) / 4096;

        _adc.Channel=ADC_CHANNEL_6;
        _adc.Rank=1;
        _adc.SamplingTime=ADC_SAMPLETIME_239CYCLES_5;
        HAL_ADC_ConfigChannel(&hadc1,&_adc);
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1,10);
        vTaskDelay(5);
        Get_ADCData.light	= (float)(HAL_ADC_GetValue(&hadc1) *100) / 4096;
		xQueueSend(READC_Queue, &Get_ADCData, 0); /* code */	
	}
}
void ADC_GETDATA_Thread_Start(void *PThread, StackType_t *PThread_Stack, int LMotor_Stack)
{
	xTaskCreateStatic(ADC_GETDATA_Thread,
					  "READ_ADC",
					  LMotor_Stack,
					  NULL,
					  (osPriority_t)osPriorityNormal,
					  PThread_Stack,
					  PThread);
}
