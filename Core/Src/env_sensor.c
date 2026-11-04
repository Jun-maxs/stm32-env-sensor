#include "env_sensor.h"
#include "dht11.h"
#include "bh1750.h"
#include "mq135.h"

void EnvSensor_Clear(Environment_Data_t *data)
{
  if (data == 0)
  {
    return;
  }

  data->temperature = 0.0f;
  data->humidity = 0.0f;
  data->light = 0.0f;
  data->air_quality = 0U;
  data->air_level = AIR_LEVEL_GOOD;

  data->dht11_valid = 0U;
  data->bh1750_valid = 0U;
  data->mq135_valid = 0U;
}

/*
 * 初始化所有采集模块。
 *
 * 这个函数是整个传感器采集层的统一入口。
 * main.c在CubeMX外设初始化完成后调用一次即可。
 * 每个传感器仍然保留自己的Init函数，因此也可以单独测试。
 */
Sensor_Status_t EnvSensor_Init(Environment_Data_t *data)
{
  Sensor_Status_t result = SENSOR_OK;

  EnvSensor_Clear(data);

  if (DHT11_Init() != SENSOR_OK)
  {
    result = SENSOR_ERROR;
  }

  if (BH1750_Init() != SENSOR_OK)
  {
    result = SENSOR_ERROR;
  }

  if (MQ135_Init() != SENSOR_OK)
  {
    result = SENSOR_ERROR;
  }

  return result;
}

/*
 * 读取所有传感器，并更新同一个环境数据结构体。
 *
 * 设计思路：
 * - DHT11模块只负责温湿度；
 * - BH1750模块只负责光照；
 * - MQ135模块只负责空气质量ADC值；
 * - env_sensor.c负责把多个结果组织起来，供显示、上传和主程序使用。
 */
void EnvSensor_ReadAll(Environment_Data_t *data)
{
  float temperature = 0.0f;
  float humidity = 0.0f;
  float light = 0.0f;
  uint16_t air_quality = 0U;

  if (data == 0)
  {
    return;
  }

  if (DHT11_Read(&temperature, &humidity) == SENSOR_OK)
  {
    /* 只有本次采集有效，才更新温湿度数据。 */
    data->temperature = temperature;
    data->humidity = humidity;
    data->dht11_valid = 1U;
  }
  else
  {
    data->dht11_valid = 0U;
  }

  if (BH1750_Read(&light) == SENSOR_OK)
  {
    /* 光照值已经在bh1750.c内部换算为Lux。 */
    data->light = light;
    data->bh1750_valid = 1U;
  }
  else
  {
    data->bh1750_valid = 0U;
  }

  if (MQ135_Read(&air_quality) == SENSOR_OK)
  {
    /* MQ135返回ADC平均值，等级由简单阈值换算得到。 */
    data->air_quality = air_quality;
    data->air_level = MQ135_GetLevel(air_quality);
    data->mq135_valid = 1U;
  }
  else
  {
    data->mq135_valid = 0U;
  }
}
