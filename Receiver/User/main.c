#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Key.h"
#include "OLED.h"
#include "PWM.h"
#include "AD.H"
#include "Data.h"

int main(void)
{
	//初始化
	Data_Init();
	
	while(1)
	{
		Data_Receive();  // 持续监听并接收数据
	}
}

