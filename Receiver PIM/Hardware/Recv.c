#include "stm32f10x.h"
#include "OLED.h"
#include "Delay.h"


#define DATA_PORT     GPIOA
#define DATA_PIN      GPIO_Pin_2
#define TH_SHORT_US   600U
#define TH_LONG_US    1200U
#define MAX_FRAME     32
#define BLOCK_TIMEOUT_US 5000U    	//超过5ms就自动恢复
									
volatile uint8_t frame_buf[MAX_FRAME];
volatile uint8_t frame_len   = 0;
volatile uint8_t frame_ready = 0;

// 状态机变量
static uint8_t  prev_level  = 1;    // 上一次读到的电平，初始空闲为高
static uint32_t last_rise_time;     // 上一次上升沿的 TIM3 计数
static uint32_t last_edge_time;
static uint8_t  in_frame    = 0;    // 0=同步头阶段,1=数据阶段
static uint8_t  sync_count  = 0;    // 已接到的 0xAA 个数
static uint8_t  bit_count   = 0;    // 当前字节已累积的比特数
static uint8_t  byte_acc    = 0;    // 当前字节累加寄存器
static uint8_t  byte_index  = 0;    // 已写入 frame_buf 的字节数

/**
 * @brief 轮询版初始化：PA2 上拉 + TIM3 μs 计时
 */
void Recv_Init(void) {
    // 1. 开 GPIOA、AFIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    // 2. PA2 = 内部上拉输入（空闲高，脉冲拉低）
    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Pin   = DATA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DATA_PORT, &gpio);

    // 3. TIM3 配置为 1MHz 自由计数器（1 μs/计数）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseInitTypeDef tb = {0};
    tb.TIM_Period        = 0xFFFF;
    tb.TIM_Prescaler     = SystemCoreClock/1000000 - 1;
    tb.TIM_ClockDivision = 0;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tb);
    TIM_Cmd(TIM3, ENABLE);
    last_rise_time = TIM3->CNT;  // 记录起始参考
	last_edge_time = last_rise_time;
	
    // 4. 重置状态机
	prev_level   = 1;  
    in_frame    = 0;
    sync_count  = 0;
    bit_count   = 0;
    byte_acc    = 0;
    byte_index  = 0;
    frame_len   = 0;
    frame_ready = 0;
}

/**
 * @brief 轮询检测一帧并解码
 *        返回时 frame_ready=1 且 frame_buf/len 已填好
 */
void Recv_PollingFrame(void) {
    while (!frame_ready) {
        // === 1. 检查“遮挡超时” ===
        uint32_t now = TIM3->CNT;
        uint32_t idle = (now >= last_edge_time)
                      ? (now - last_edge_time)
                      : (0x10000 + now - last_edge_time);
        if (idle > BLOCK_TIMEOUT_US) {
            // 光路长时间无边沿跳变，判定为“遮挡”
            prev_level  = 1;
            in_frame    = 0;
            sync_count  = 0;
            bit_count   = 0;
            byte_acc    = 0;
            frame_len   = 0;
            byte_index  = 0;
            // 更新 last_edge_time，避免连续重复触发
            last_edge_time = now;
            // 可选：在 OLED 上给提示
            OLED_ShowString(4, 1, "No Signal");
        }
        uint8_t cur = GPIO_ReadInputDataBit(DATA_PORT, DATA_PIN);

        // 1) 检测 0→1 上升沿：记录脉冲结束时间
        if (prev_level == 0 && cur == 1) {
            last_rise_time = TIM3->CNT;
			last_edge_time   = last_rise_time;
        }
        // 2) 检测 1→0 下降沿：脉冲开始，计算间隔并判 0/1
        else if (prev_level == 1 && cur == 0) {
            uint32_t now2 = TIM3->CNT;
            uint32_t dt  = (now2 >= last_rise_time)
                         ? (now2 - last_rise_time)
                         : (0x10000 + now2 - last_rise_time);
			last_edge_time = now2;
            uint8_t bit = (dt < 900 ? 0 : 1);

            
            // 同步头阶段
			if (!in_frame) {
				byte_acc = (byte_acc << 1) | bit;
				if (++bit_count == 8) {
					OLED_ShowHexNum(1,7,byte_acc, 2);
					if (byte_acc == 0xAA) {
						if (++sync_count == 2) {
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
			}
			// 数据阶段
			else {
				byte_acc = (byte_acc << 1) | bit;
				if (++bit_count == 8) {
					if (frame_len == 0) {
						// 第一个字节 = 长度
						frame_len = byte_acc;
					}
					else if (byte_index < frame_len) {
						// 收到数据
						frame_buf[byte_index++] = byte_acc;
					}
					else {
						// 校验：累加前面所有字节
						uint8_t sum = 0;
						for (uint8_t i = 0; i < frame_len; i++)
							sum += frame_buf[i];
						// 在 OLED 上显示整个帧
						//byte_acc -= 1;
						OLED_Clear();
						OLED_ShowHexNum(1,1,0xAA,2);
						OLED_ShowHexNum(1,4,0xAA,2);
						OLED_ShowNum   (2,1,frame_len,2);
						OLED_ShowNum   (2,4,sum, 2);
						OLED_ShowNum   (2,7,byte_acc,2);
						OLED_ShowChar(3,1,(char)frame_buf[0]);
						OLED_ShowChar(3,3,(char)frame_buf[1]);
						OLED_ShowChar(3,5,(char)frame_buf[2]);
						OLED_ShowChar(3,7,(char)frame_buf[3]);
						
						OLED_ShowString(4,1,(sum == byte_acc)?"OK":"ERR");
						
						if(sum == byte_acc)
						{
							frame_ready = 1;
						}
						else{
							OLED_ShowString(1,1,"Restarting...");
							Delay_ms(2000);
							OLED_Clear();
							OLED_ShowString(1,1,"running...");
							prev_level 	= 1;
							in_frame 	= 0;
							sync_count 	= 0;
							bit_count 	= 0;
							frame_len 	= 0;
							byte_index 	= 0;
							frame_ready = 0;
						}
					}
					bit_count = 0;
					byte_acc  = 0; 
                }
            }
        }

        prev_level = cur;
    }
}
