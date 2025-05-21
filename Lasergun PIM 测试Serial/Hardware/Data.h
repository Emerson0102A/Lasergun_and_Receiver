#ifndef __DATA_H
#define __DATA_H
#include <stdint.h>

void Data_Init(void);                          // 初始化PA2为数据输出引脚
void Data_SendString(const char *str); // 发送字符串数据
void Data_Test(void);

#endif
