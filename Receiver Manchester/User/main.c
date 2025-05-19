#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Recv.h"

int main(void)
{
	SystemInit();
	Recv_Init();
	OLED_Init();
	
	OLED_ShowString(1,1,"Running...");
	//OLED_ShowNum(2,1,3,4);
	
	Recv_PollingFrame();  
	
	
	
	while(1)
	{
		
				
	}
}

