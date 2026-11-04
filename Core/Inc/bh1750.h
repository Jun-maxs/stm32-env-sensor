#ifndef __BH1750_H
#define __BH1750_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "sensor_status.h"

/*
 * BH1750软件IIC接线：
 * SCL -> PB6
 * SDA -> PB7
 */
#define BH1750_SCL_PORT GPIOB
#define BH1750_SCL_PIN  GPIO_PIN_6
#define BH1750_SDA_PORT GPIOB
#define BH1750_SDA_PIN  GPIO_PIN_7

/* 初始化软件IIC总线，并设置BH1750测量模式。 */
Sensor_Status_t BH1750_Init(void);

/* 读取光照强度，并将原始数据换算为Lux。 */
Sensor_Status_t BH1750_Read(float *light_lux);

#ifdef __cplusplus
}
#endif

#endif
