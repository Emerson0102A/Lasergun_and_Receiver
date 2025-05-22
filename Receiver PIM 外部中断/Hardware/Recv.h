#ifndef __RECV_H
#define __RECV_H

#include "stm32f10x.h"
#include <stdint.h>

/**
 * @brief 初始化接收模块：配置 PA2 内部上拉输入与 TIM3 微秒计时
 */
void Recv_Init(void);

/**
 * @brief 轮询接收并解码一帧数据
 *
 * 调用此函数后，若成功接收到一帧，则会在 OLED 上自动
 * 显示同步头、长度、数据和校验结果，并设置 frame_ready=1。
 *
 * 返回前已完成所有显示，若需要处理接收到的数据，可访问:
 *   frame_len 和 frame_buf[0..frame_len-1]
 */
void Recv_PollingFrame(void);

/** 接收完成标志：1 表示已解码并显示一帧 */
extern volatile uint8_t frame_ready;

/** 当前接收帧的数据长度（字节数） */
extern volatile uint8_t frame_len;

/** 接收缓冲区，存放解码后的一帧数据（frame_len 字节） */
#define MAX_FRAME 32
extern volatile uint8_t frame_buf[MAX_FRAME];
extern volatile uint8_t last_sync_byte;
#endif // __RECV_H
