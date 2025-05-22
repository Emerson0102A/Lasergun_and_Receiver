#include "stm32f10x.h"    // Device header
#include "Delay.h"
#include "LED.h"
#include "Data.h"
#include "Serial.h"

volatile uint8_t led1_state = 0;    // 0:熄灭, 1:点亮
volatile uint8_t pb11_forced = 0;   // 0:未强制, 1:PB11强制点亮

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t Key_GetNum(void)
{
	static uint8_t last_pb1 = 1, last_pb11 = 1; // 默认未按下
    uint8_t key_event = 0;
    
    // 检测PB1（切换功能）
    uint8_t current_pb1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
    if (last_pb1 == 1 && current_pb1 == 0) { // 下降沿触发
        Delay_ms(20);//消抖
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0) {
            key_event = 1; // PB1按下事件
        }
    }
    last_pb1 = current_pb1;
    
    // 检测PB11（强制点亮功能）
    uint8_t current_pb11 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
    if (last_pb11==1 && current_pb11==0) {
        Delay_ms(20);//消抖
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)==0) {//按下是0，松开时1
            key_event = 2;
            
        }
    }
    last_pb11 = current_pb11;
    
    return key_event;
}

void Key_Control(void){
	//按键
	uint8_t key_event = Key_GetNum();
      
    // 处理PB11强制控制（优先级最高）
    if (key_event == 2) { 
        LED1_ON(); 
		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0) {
            Data_SendString(Serial_RxPacket);
            Delay_ms(50);  // 间隔 50 ms 重发一次
        }
        // 松手后跳出循环
        LED1_OFF();
        
    }
        
    // 处理PB1切换（仅在未强制时生效）
    else if (key_event == 1) { 
        LED1_ON();
		int id = 0;
		while(id < 50){
			Data_SendString(Serial_RxPacket);
			id++;
		}
//		while(1){
//			
//			Data_Test();
//		}
        //Data_SendString(Serial_RxPacket);
        LED1_OFF();
    }
}
