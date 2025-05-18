#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Recv.h"

int main(void)
{
	
	Data_Init();
	OLED_Init();
	
	OLED_ShowString(1,1,"Running...");
	
	
	while(1)
	{
		
				
	}
}

