#ifndef __DHT11_H
#define __DHT11_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "sensor_status.h"

/*
 * DHT11接线：
 * DATA -> PG11
 */
#define DHT11_GPIO_PORT GPIOG
#define DHT11_GPIO_PIN  GPIO_PIN_11

/* 初始化DHT11数据线。 */
Sensor_Status_t DHT11_Init(void);

/* 读取温度和湿度，返回值用于判断数据是否有效。 */
Sensor_Status_t DHT11_Read(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif
