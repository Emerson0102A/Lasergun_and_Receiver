#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#define DATA_PIN     GPIO_Pin_0//TIM_Channel_1                // 使用PA2作为数据输出
#define DATA_PORT    GPIOA
#define BIT_TIME_US  833                       // 波特率1200bps (1/1200 ≈ 833μs/bit)

// 初始化PA2为推挽输出模式
void Data_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStruct.GPIO_Pin = DATA_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DATA_PORT, &GPIO_InitStruct);
    
    GPIO_ResetBits(DATA_PORT, DATA_PIN);        // 初始状态为低电平
}

// 发送单个比特（OOK调制）
static void SendBit(uint8_t bit) {
    if (bit) {
        TIM_CCxCmd(TIM2, TIM_Channel_1, ENABLE);
    } else {
        TIM_CCxCmd(TIM2, TIM_Channel_1, DISABLE);
    }
    Delay_us(BIT_TIME_US);
}
// 发送一个字节（高位在前）
static void SendByte(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        SendBit((byte >> i) & 0x01);            // 从最高位开始发送
    }
}

// 发送完整数据帧（同步头 + 数据 + 校验和）
void Data_SendString(const char *str, uint8_t length) {
    // 1. 发送2字节同步头 0xAA
    SendByte(0xAA);
    SendByte(0xAA);
    
    // 2. 发送数据长度
    SendByte(length);
    
    // 3. 发送数据内容
    for (uint8_t i = 0; i < length; i++) {
        SendByte(str[i]);
    }
    
    // 4. 发送校验和（简单累加和）
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++) {
        checksum += str[i];
    }
    SendByte(checksum);
    
    // 发送完成后复位引脚
    TIM_CCxCmd(TIM2, TIM_Channel_1, DISABLE);
}
