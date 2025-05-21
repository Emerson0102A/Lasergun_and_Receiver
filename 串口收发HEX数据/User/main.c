

#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "Key.h"

uint8_t KeyNum;

uint8_t RxData;

int main(void)
{

	OLED_Init();
	Serial_Init();
	Key_Init();
	
	Serial_TxPacket[0] = 0x01;
	Serial_TxPacket[0] = 0x02;
	Serial_TxPacket[0] = 0x03;
	Serial_TxPacket[0] = 0x04;
	
	Serial_SendPacket();
	
	while(1)
	{
		if (Serial_GetRxFlag() == 1)
		{
			OLED_ShowHexNum(1,1,Serial_RxPacket[0],2);
			OLED_ShowHexNum(1,4,Serial_RxPacket[1],2);
			OLED_ShowHexNum(1,7,Serial_RxPacket[2],2);
			OLED_ShowHexNum(1,10,Serial_RxPacket[3],2);
		}
		
		
		
	}
}
