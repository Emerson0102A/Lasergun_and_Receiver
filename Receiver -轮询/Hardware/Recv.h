#ifndef __RECV_H
#define __RECV_H

#include "stm32f10x.h"
#include <stdint.h>

/**
 * @brief 初始化数据接收模块
 *
 * 配置 PA2 为浮空输入，
 * 并设置 EXTI2（下降沿触发）＋NVIC，
 * 同时清空数字滤波缓冲区。
 */
void Recv_Init(void);
void Recv_PollingFrame(void);


#endif 
