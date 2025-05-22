#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Recv.h"

int main(void)
{
    SystemInit();
    OLED_Init();
    Recv_Init();
    OLED_ShowString(1,1,"Waiting frame...");

    while (1) {
        if (last_sync_byte != 0) {
            OLED_ShowString(4,1,"Sync:");
            OLED_ShowHexNum(4,6,last_sync_byte,2);
            last_sync_byte = 0;
        }
        if (frame_ready) {
            // 收到一帧 OK
            OLED_Clear();
            OLED_ShowString(1,1,"Frame OK");
            // 显示长度
            OLED_ShowNum(2,1, frame_len, 2);
            // 校验和
            uint8_t sum = 0;
            for (uint8_t i = 0; i < frame_len; i++) sum += frame_buf[i];
            OLED_ShowNum(2,4, sum, 2);
            // 数据
            for (uint8_t i = 0; i < frame_len; i++) {
                OLED_ShowHexNum(3, 1+3*i, frame_buf[i], 2);
            }
            frame_ready = 0;  // 清标志，继续接收下帧
            OLED_ShowString(4,1,"Waiting...");
        }else{
            // 收到一帧 OK
            OLED_Clear();
            OLED_ShowString(1,1,"Error");
            // 显示长度
            OLED_ShowNum(2,1, frame_len, 2);
            // 校验和
            uint8_t sum = 0;
            for (uint8_t i = 0; i < frame_len; i++) sum += frame_buf[i];
            OLED_ShowNum(2,4, sum, 2);
            // 数据
            for (uint8_t i = 0; i < frame_len; i++) {
                OLED_ShowHexNum(3, 1+3*i, frame_buf[i], 2);
            }
            frame_ready = 0;  // 清标志，继续接收下帧
            OLED_ShowString(4,1,"Waiting...");
		}
    }
}
