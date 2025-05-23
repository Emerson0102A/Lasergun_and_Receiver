#ifndef __KEY_H
#define __KEY_H

extern volatile uint8_t led1_state;    // 0:熄灭, 1:点亮
extern volatile uint8_t pb11_forced;   // 0:未强制, 1:PB11强制点亮
extern volatile uint8_t laser_state;   // 0=激光熄灭, 1=激光点亮

void Key_Init(void);
uint8_t Key_GetNum(void);
void Key_Control(void);

#endif
