#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define DATA_PORT    GPIOA
#define DATA_PIN     GPIO_Pin_2
#define BIT_TIME_US  833     // 1200 bps

// —— 数字滤波：滑动窗口多数投票 N=3 —— 
#define FILTER_LEN  3
static uint8_t filter_buf[FILTER_LEN];
static uint8_t filter_idx = 0;

volatile uint8_t got_wave = 0;

/**
 * @brief 3 点多数投票滤波
 * @param raw 新采样 (0/1)
 * @return  滤波后 (0/1)
 */
static uint8_t DigitalFilter(uint8_t raw) {
    filter_buf[filter_idx] = raw;
    if (++filter_idx >= FILTER_LEN) filter_idx = 0;
    int sum = 0;
    for (int i = 0; i < FILTER_LEN; i++) sum += filter_buf[i];
    return (sum > FILTER_LEN/2) ? 1 : 0;
}

/**
 * @brief 读取一个比特，反相 + 数字滤波
 */
static uint8_t Data_ReadBit(void) {
	uint8_t votes[3];
	
	Delay_us(BIT_TIME_US / 4);
	votes[0] = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
	
	Delay_us(BIT_TIME_US / 4);
	votes[1] = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
	
	Delay_us(BIT_TIME_US / 4);
	votes[2] = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
	
	Delay_us(BIT_TIME_US / 4);
    uint8_t raw = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
	
	
    return (votes[0] + votes[1] + votes[2] >= 2) ? 1 : 0;
	
	
}



/**
 * @brief 读取一个字节，MSB 先行
 */
static uint8_t Data_ReadByte(void) {
    uint8_t b = 0;
    for (int i = 7; i >= 0; i--) {
        if (Data_ReadBit()) b |= (1 << i);
        Delay_us(BIT_TIME_US);
    }
    return b;
}

/**
 * @brief EXTI2 中断入口：检测到下降沿时触发
 */

void EXTI2_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line2) == RESET) return;
	//OLED_ShowString(1,1,"getWave");
    EXTI_ClearITPendingBit(EXTI_Line2);
    // 禁用后续中断，避免重入
    EXTI->IMR &= ~EXTI_Line2;
	

    // 对齐到第 1 个比特中点
    Delay_us(BIT_TIME_US/20);
	
	/*for (int i = 0; i < FILTER_LEN; i++) {
		filter_buf[i] = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
		Delay_us(BIT_TIME_US);
	}
	filter_idx = 0;*/
	
	// 1) 读两个同步头
	uint8_t sync1 = Data_ReadByte();
	uint8_t sync2 = Data_ReadByte();
	char dbg[16];
	sprintf(dbg, "%02X %02X", sync1, sync2);
	OLED_ShowString(2,1, dbg);

    
	
    if (sync1 == 0xAA && sync2 == 0xAA) {
        // 2) 读长度
        uint8_t len = Data_ReadByte();
		OLED_ShowNum(3,1,len,1);
        // 上限保护
        if (len > 0 && len <= 255) {
            uint8_t buf[256];
            // 3) 读数据j 
            for (uint8_t i = 0; i < len; i++) {
                buf[i] = Data_ReadByte();
            }
            // 4) 读校验
            uint8_t csum = Data_ReadByte();
			
			
			
            // 校验
            uint8_t sum = 0;
            for (uint8_t i = 0; i < len; i++) sum += buf[i];
            if (sum == csum) {
				buf[len] = '\0';
				OLED_Clear();  
				OLED_ShowString(2, 1, "Successful!");
				
            }else{
				OLED_Clear();
				OLED_ShowString(2, 1, "Failed");
			}
			OLED_ShowString(4, 1, (char*)buf);
        }
    }
	Delay_us(BIT_TIME_US * 10);  
    // 重新使能中断，准备下一帧
    EXTI->IMR |= EXTI_Line2;
}

/**
 * @brief 初始化 PA2 输入 + EXTI2 + NVIC
 *        使能下降沿触发外部中断
 */
void Data_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // 1) 打时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA  |
                           RCC_APB2Periph_AFIO, ENABLE);

    // 2) PA2 
    GPIO_InitStruct.GPIO_Pin  = DATA_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DATA_PORT, &GPIO_InitStruct);

    // 3) PA2→EXTI2 复用
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);

    // 4) EXTI2: 下降沿触发
    EXTI_InitStruct.EXTI_Line    = EXTI_Line2;
    EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStruct);

    // 5) NVIC 使能 EXTI2_IRQn
    NVIC_InitStruct.NVIC_IRQChannel                   = EXTI2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // 清空 filter_buf，避免启动时读到脏数据
    memset(filter_buf, 0, sizeof(filter_buf));
}

