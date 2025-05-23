/*说明：
Data直接接在+极上
A0 38kHz载波
A1 供电


*/


#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Key.h"
#include "OLED.h"
#include "PWM.h"
#include "Data.h"
#include "Serial.h"




int main(void)
{
	
	LED_Init();
	Key_Init();
	OLED_Init();
	PWM_Init();
	Data_Init();
	Serial_Init();
	
	PWM_SetCompare1(50);
	
	
	
	
	while(1)
	{
		//按键
		Key_Control();
		
		if(Serial_GetRxFlag() == 1)
		{
			OLED_ShowString(4, 1, "                ");
			OLED_ShowString(4, 1, Serial_RxPacket);
		}
		
		
		
		
		
		
		
		
	}
}
