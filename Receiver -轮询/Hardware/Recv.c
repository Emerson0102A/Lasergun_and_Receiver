#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include <stdint.h>

#define DATA_PORT    GPIOA
#define DATA_PIN     GPIO_Pin_2
#define BIT_TIME_US  833U    // 1200 bps

/**
 * @brief 初始化 PA2 为浮空输入（轮询模式）
 */
void Recv_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin   = DATA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DATA_PORT, &gpio);
}

/**
 * @brief 在一个比特周期内三点多数投票读取一个比特
 * @return 0 或 1
 */
static uint8_t Recv_ReadBit(void) {
    uint8_t votes[3];
    for (int i = 0; i < 3; ++i) {
        Delay_us(BIT_TIME_US / 4);
        votes[i] = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
    }
    Delay_us(BIT_TIME_US / 4);
    int sum = votes[0] + votes[1] + votes[2];
	uint8_t res = (sum >= 2) ? 1 : 0;
	if(res){
		Delay_us(100);
	}
    return res;
}

static uint8_t Recv_ReadBit_Raw(void) {
	return GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
}

/**
 * @brief MSB-first 读取一个字节
 */
static uint8_t Recv_ReadByte(void) {
    uint8_t b = 0;
    for (int i = 7; i >= 0; --i) {
		int res = Recv_ReadBit();
		if(res){
			b |= (Recv_ReadBit() << i);
			
        }
		
    }
    return b;
}

/**
 * @brief 在主循环中调用，轮询检测下降沿并接收一帧
 */
void Recv_PollingFrame(void) {
    static uint8_t prev = 1;
    uint8_t cur = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
    // 检测到高->低：起始下降沿
    if (prev == 1 && cur == 0) {
        // 对齐到第一个数据位中点
        //Delay_us(3 * BIT_TIME_US);

        // 读同步头
        uint8_t sync1 = Recv_ReadByte();
        uint8_t sync2 = Recv_ReadByte();
        // 显示同步头
        OLED_Clear();
        OLED_ShowHexNum(1, 1, sync1, 2);
        OLED_ShowHexNum(1, 6, sync2, 2);

        if (sync1 == 0xAA && sync2 == 0xAA) {
            // 读长度
            uint8_t len = Recv_ReadByte();
            OLED_ShowNum(2, 1, len, 4);
            // 读数据
            for (uint8_t i = 0; i < len; ++i) {
                uint8_t d = Recv_ReadByte();
                // 每行最多显示 8 字节
                uint8_t row = 3 + (i / 8);
                uint8_t col = (i % 8) + 1;
                OLED_ShowNum(row, col, d, 2);
            }
            // 读并校验
            uint8_t csum = Recv_ReadByte();
            uint8_t sum = 0;
            // 重新计算校验时，需要缓存数据或再读
            OLED_ShowString(5, 1, (sum == csum) ? "OK" : "ERR");
        }
        // 等待线路回到高电平，避免重复检测
        while (GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN) == 0);
        // 小延时消抖
        Delay_us(BIT_TIME_US);
    }
    prev = cur;
}
