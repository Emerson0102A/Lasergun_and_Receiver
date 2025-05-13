#include "stm32f10x.h"    // Device header
#include "Delay.h"
#include "LED.h"

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
            while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0); // 等待松开
        }
    }
    last_pb1 = current_pb1;
    
    // 检测PB11（强制点亮功能）
    uint8_t current_pb11 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
    if (current_pb11 == 0) { // 持续检测低电平
        key_event = 2; // PB11按下事件
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
        pb11_forced = 1; // 标记强制点亮
    } else if (pb11_forced) { 
        LED1_OFF();
        pb11_forced = 0; // 清除强制标志
        // 恢复PB1切换前的状态
        if (led1_state) LED1_ON();
    }
        
    // 处理PB1切换（仅在未强制时生效）
    if (key_event == 1 && !pb11_forced) { 
        led1_state = !led1_state;
        if (led1_state) {
            LED1_ON();
        } else {
            LED1_OFF();
        }
    }
}
