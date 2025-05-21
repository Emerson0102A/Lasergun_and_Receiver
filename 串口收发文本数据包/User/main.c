#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"

uint8_t RxData;

int main(void)
{

	OLED_Init();
	Serial_Init();
	
	
	
	
	while(1)
	{
		if(Serial_GetRxFlag() == 1)
		{
			OLED_ShowString(4, 1, "                ");
			OLED_ShowString(4, 1, Serial_RxPacket);
		}
		
		
		
	}
}
