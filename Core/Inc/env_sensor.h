#ifndef __ENV_SENSOR_H
#define __ENV_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor_status.h"
#include "mq135.h"

typedef struct
{
  /* 环境采集结果 */
  float temperature;
  float humidity;
  float light;
  uint16_t air_quality;
  Air_Level_t air_level;

  /* 有效标志：记录本次读取是否成功 */
  uint8_t dht11_valid;
  uint8_t bh1750_valid;
  uint8_t mq135_valid;
} Environment_Data_t;

/* 清空环境数据和有效标志。 */
void EnvSensor_Clear(Environment_Data_t *data);

/* 统一初始化DHT11、BH1750和MQ135模块。 */
Sensor_Status_t EnvSensor_Init(Environment_Data_t *data);

/* 统一读取所有传感器，并更新Environment_Data_t结构体。 */
void EnvSensor_ReadAll(Environment_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif
