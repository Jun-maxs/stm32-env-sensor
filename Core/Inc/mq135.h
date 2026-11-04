#ifndef __MQ135_H
#define __MQ135_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "sensor_status.h"

#define MQ135_SAMPLE_COUNT 10U

typedef enum
{
  /* 以下等级是课堂演示阈值，不是经过标定的ppm浓度。 */
  AIR_LEVEL_GOOD = 0,
  AIR_LEVEL_NORMAL,
  AIR_LEVEL_WARNING
} Air_Level_t;

/* 对MQ135模拟输入对应的ADC进行校准。 */
Sensor_Status_t MQ135_Init(void);

/* 多次采样并求平均值，读取MQ135模拟量。 */
Sensor_Status_t MQ135_Read(uint16_t *adc_value);

/* 将ADC值转换为简单的空气质量等级。 */
Air_Level_t MQ135_GetLevel(uint16_t adc_value);

#ifdef __cplusplus
}
#endif

#endif
