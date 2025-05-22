#include "stm32f10x.h"
#include "Recv.h"
#include "Delay.h"

#define DATA_PORT       GPIOA
#define DATA_PIN        GPIO_Pin_2
#define DATA_PIN_SOURCE GPIO_PinSource2
#define DATA_GPIO_PORT  GPIOA
#define EXTI_LINE       EXTI_Line2
#define NVIC_IRQ_CH     EXTI2_IRQn
#define TH_SHORT_US     600U
#define TH_LONG_US      1200U
#define MAX_FRAME       32
#define BLOCK_TIMEOUT_US 5000U

volatile uint8_t frame_buf[MAX_FRAME];
volatile uint8_t frame_len   = 0;
volatile uint8_t frame_ready = 0;
volatile uint8_t last_sync_byte = 0;
// 状态机变量
static uint8_t  prev_level   = 1;
static uint32_t last_rise_time;
static uint32_t last_edge_time;
static uint8_t  in_frame     = 0;
static uint8_t  sync_count   = 0;
static uint8_t  bit_count    = 0;
static uint8_t  byte_acc     = 0;
static uint8_t  byte_index   = 0;

void Recv_Init(void) {
    // 1. GPIO + AFIO + EXTI 初始化
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin  = DATA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DATA_PORT, &gpio);
    // AFIO: PA2->EXTI2
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, DATA_PIN_SOURCE);
    EXTI_InitTypeDef exti = {0};
    exti.EXTI_Line    = EXTI_LINE;
    exti.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    // NVIC
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel                   = NVIC_IRQ_CH;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    // 2. TIM3 微秒计时
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseInitTypeDef tb = {0};
    tb.TIM_Period        = 0xFFFF;
    tb.TIM_Prescaler     = SystemCoreClock/1000000 - 1;
    tb.TIM_ClockDivision = 0;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tb);
    TIM_Cmd(TIM3, ENABLE);

    // 状态机复位
    prev_level   = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
    last_rise_time = TIM3->CNT;
    last_edge_time = last_rise_time;
    in_frame     = 0;
    sync_count   = 0;
    bit_count    = 0;
    byte_acc     = 0;
    byte_index   = 0;
    frame_len    = 0;
    frame_ready  = 0;
}

// EXTI2 中断处理函数
void EXTI2_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_LINE)) {
        uint32_t now = TIM3->CNT;
        uint8_t cur = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);
        // 检测上升沿
        if (prev_level == 0 && cur == 1) {
            last_rise_time = now;
            last_edge_time = now;
        }
        // 检测下降沿
        else if (prev_level == 1 && cur == 0) {
            uint32_t dt = (now >= last_rise_time)
                        ? (now - last_rise_time)
                        : (0x10000 + now - last_rise_time);
            last_edge_time = now;
            uint8_t bit = (dt < ((TH_SHORT_US + TH_LONG_US)/2)) ? 0 : 1;

            if (!in_frame) {
                byte_acc = (byte_acc << 1) | bit;
                if (++bit_count == 8) {
					last_sync_byte = byte_acc;
                    if (byte_acc == 0xAA) {
                        if (++sync_count >= 2) {
                            in_frame   = 1;
                            frame_len  = 0;
                            byte_index = 0;
                        }
                    } else {
                        sync_count = 0;
                    }
                    bit_count = 0;
                    byte_acc  = 0;
                }
            } else {
                byte_acc = (byte_acc << 1) | bit;
                if (++bit_count == 8) {
                    if (frame_len == 0) {
                        frame_len = byte_acc;
                    } else if (byte_index < frame_len) {
                        frame_buf[byte_index++] = byte_acc;
                    } else {
                        // 校验
                        uint8_t sum = 0;
                        for (uint8_t i = 0; i < frame_len; i++) sum += frame_buf[i];
                        if (sum == byte_acc) {
                            frame_ready = 1;  // 一帧就绪
                        }
                        // 不论 OK/ERR，都重置状态机等待下一帧
                        prev_level = cur;
                        in_frame   = 0;
                        sync_count = 0;
                        bit_count  = 0;
                        byte_acc   = 0;
                        byte_index = 0;
                        // 不清 frame_buf/frame_len, 留给主函数处理
                        EXTI_ClearITPendingBit(EXTI_LINE);
                        return;
                    }
                    bit_count = 0;
                    byte_acc  = 0;
                }
            }
        }
        prev_level = cur;
        EXTI_ClearITPendingBit(EXTI_LINE);
    }
}
