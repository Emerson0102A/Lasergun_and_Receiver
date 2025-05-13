#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"

#define DATA_PIN GPIO_Pin_2  // 连接到接收模块的 OUT 引脚
#define DATA_PORT GPIOA      // 假设连接到 GPIOA

#define BIT_TIME_US 833      // 1 bit 等待时间（根据波特率，833 微秒对应 1200bps）

// 初始化 GPIO 作为输入
void Data_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStruct.GPIO_Pin = DATA_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 设置为浮动输入
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DATA_PORT, &GPIO_InitStruct);
}

// 读取一个 bit（0 或 1）
uint8_t Data_ReadBit(void) {
    return GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
}

// 从接收模块读取一个字节（8 位）
uint8_t Data_ReadByte(void) {
    uint8_t byte = 0;
    
    for (int i = 7; i >= 0; i--) {
        if (Data_ReadBit()) {
            byte |= (1 << i);  // 如果是高电平，则设置相应位
        }
        Delay_us(BIT_TIME_US);  // 等待 833 微秒
    }
    
    return byte;
}

// 解码并输出接收到的数据
void Data_Receive(void) {
    uint8_t sync1, sync2, length, checksum;
    uint8_t data[256];  // 假设最多接收 256 字节数据
    uint8_t i;

    // 1. 等待同步头 0xAA 0xAA
    sync1 = Data_ReadByte();
    sync2 = Data_ReadByte();
    
    if (sync1 != 0xAA || sync2 != 0xAA) {
        return;  // 如果没有同步头，直接返回
    }

    // 2. 读取数据长度
    length = Data_ReadByte();

    // 3. 读取数据
    for (i = 0; i < length; i++) {
        data[i] = Data_ReadByte();
    }

    // 4. 读取校验和
    checksum = Data_ReadByte();

    // 校验和验证（假设简单的累加和校验）
    uint8_t computed_checksum = 0;
    for (i = 0; i < length; i++) {
        computed_checksum += data[i];
    }

    // 校验是否正确
    if (computed_checksum == checksum) {
        OLED_ShowString(1, 1, "success");
		OLED_ShowString(2, 1, (char *)data);
		
    } else {
        OLED_ShowString(1,1,"fail");
    }
}


