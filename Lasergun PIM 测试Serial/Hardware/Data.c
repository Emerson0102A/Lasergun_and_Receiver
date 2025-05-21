#include "stm32f10x.h"
#include "Delay.h"
#include "PWM.h"
#include "Data.h"
#include <string.h>

// —— PIM 参数 ——
// 脉冲本身持续 200 µs（载波 38 kHz）：
#define PULSE_WIDTH_US   200U
// 脉冲后短/长间隔：短表示 0、长表示 1
#define INTERVAL_SHORT   600U
#define INTERVAL_LONG   1400U
// 帧间空闲：在发送前让线保持低电平 3 ms，确保接收器捕捉到帧头
#define FRAME_GAP_US    3000U
// 载波脉冲的 PWM 占空比（50% 你可以根据实际调整）
#define PULSE_DUTY        50U  

// —— 基本载波控制 ——
// on 置 载波 PWM 占空比为 PULSE_DUTY；off 则关掉载波
static inline void SendRaw(uint8_t on) {
    if (on) {
        PWM_SetCompare1(PULSE_DUTY);
    } else {
        PWM_SetCompare1(0);
    }
}

void Data_Init(void) {
    // 1) 初始化载波硬件（38 kHz PWM 在 PA0）
    PWM_Init();
    // 2) 确保空闲时载波关闭
    SendRaw(0);
}

// 发送一个 PIM 比特
static void SendPIMBit(uint8_t bit) {
    // 1) 发一个固定宽度的载波脉冲
    SendRaw(1);
    Delay_us(PULSE_WIDTH_US);
    SendRaw(0);
    // 2) 根据 bit 值插入短或长间隔
    Delay_us(bit ? INTERVAL_LONG : INTERVAL_SHORT);
}

// MSB‐first 发送一个字节
static void SendPIMByte(uint8_t byte) {
    for (int i = 7; i >= 0; --i) {
        SendPIMBit((byte >> i) & 0x1);
    }
}

void Data_SendString(const char *str) {
	uint8_t length = strlen(str);
	
	
    uint8_t checksum = 0;
    // 1) 帧间隔，让接收机从上一次帧中“复位”
    Delay_us(FRAME_GAP_US);
    // 2) 同步头 0xAA 0xAA
    SendPIMByte(0xAA);  
    SendPIMByte(0xAA);
    // 3) 发送长度
    SendPIMByte(length);
    // 4) 逐字符发送，并累加校验
    for (uint8_t i = 0; i < length; ++i) {
        uint8_t b = (uint8_t)str[i];
        SendPIMByte(b);
        checksum += b;
    }
    // 5) 发送校验和
    SendPIMByte(checksum);
    // 6) 发送结束，关闭载波
    SendRaw(1);
    Delay_us(PULSE_WIDTH_US);
    SendRaw(0);
}

void Data_Test(void) {
    // 只发一个 0xAA 脉冲
    SendPIMByte(0xAA);
    SendRaw(0);
}
