/*说明：
电源直接接在+极上
A0 38kHz载波
A1 电源


*/


#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Key.h"
#include "OLED.h"
#include "PWM.h"
#include "Data.h"




int main(void)
{
	LED_Init();
	Key_Init();
	OLED_Init();
	PWM_Init();
	Data_Init();
	
	OLED_ShowString(1,1,"wuchangsheng");
	OLED_ShowString(2,1,"2024080905006");
	
	while(1)
	{
		//按键
		Key_Control();
		//PWM
		PWM_SetCompare1(50);
		
		
		
		
		
	}
}
