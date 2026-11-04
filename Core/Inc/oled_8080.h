#ifndef __OLED_8080_H
#define __OLED_8080_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "env_sensor.h"

/*
 * 正点原子0.96寸OLED模块，SSD1306驱动芯片，8080并行接口。
 *
 * 本工程接线：
 * D0-D7 : PC0-PC7
 * CS    : PD6
 * RS    : PD3
 * WR    : PG14
 * RD    : PG13
 * RST   : PG15
 */

void OLED8080_Init(void);
void OLED8080_Clear(void);
void OLED8080_ShowString(uint8_t x, uint8_t page, const char *str);
void OLED8080_ShowEnvData(const Environment_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif
