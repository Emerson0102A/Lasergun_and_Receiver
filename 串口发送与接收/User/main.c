

#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include <stdio.h>


uint8_t RxData;

int main(void)
{

	OLED_Init();
	OLED_ShowString(1,1,"RxData:");
	
	Serial_Init();
	
	
	
//	OLED_ShowString(1,1,"wuchangsheng");
//	OLED_ShowString(2,1,"2024080905006");
	
	while(1)
	{
		if(Serial_GetRxFlag() == 1)
		{
			RxData = Serial_GetRxData();
			Serial_SendByte(RxData);
			OLED_ShowHexNum(1, 8, RxData, 2);
		}
		
		
		
		
	}
}
