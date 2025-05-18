#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Data.h"

int main(void)
{
	
	Data_Init();
	OLED_Init();
	
	OLED_ShowString(1,1,"Running...");
	
	
	while(1)
	{
		
				
	}
}

