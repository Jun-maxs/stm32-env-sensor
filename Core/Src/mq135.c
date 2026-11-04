#include "mq135.h"
#include "adc.h"

static Sensor_Status_t MQ135_ReadADC(uint16_t *value)
{
  if (value == 0)
  {
    return SENSOR_ERROR;
  }

  /* 启动一次软件触发的ADC1_IN1转换。 */
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);
    return SENSOR_TIMEOUT;
  }

  *value = (uint16_t)HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  return SENSOR_OK;
}

Sensor_Status_t MQ135_Init(void)
{
  /*
   * STM32F1的ADC初始化后需要校准，以提高采样准确性。
   * CubeMX负责配置PA1/ADC1_IN1，本模块负责运行时采集。
   */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
    return SENSOR_ERROR;
  }

  return SENSOR_OK;
}

Sensor_Status_t MQ135_Read(uint16_t *adc_value)
{
  uint32_t sum = 0;
  uint16_t sample = 0;
  uint8_t i;

  if (adc_value == 0)
  {
    return SENSOR_ERROR;
  }

  /*
   * MQ135模拟输出容易波动。
   * 这里采用多次采样平均，让串口输出更稳定，便于课堂观察。
   */
  for (i = 0; i < MQ135_SAMPLE_COUNT; i++)
  {
    if (MQ135_ReadADC(&sample) != SENSOR_OK)
    {
      return SENSOR_TIMEOUT;
    }
    sum += sample;
    HAL_Delay(2);
  }

  *adc_value = (uint16_t)(sum / MQ135_SAMPLE_COUNT);
  return SENSOR_OK;
}

Air_Level_t MQ135_GetLevel(uint16_t adc_value)
{
  /*
   * 下面阈值仅用于课堂演示。
   * 如果需要准确气体浓度，MQ135需要充分预热并进行标定。
   */
  if (adc_value < 1200U)
  {
    return AIR_LEVEL_GOOD;
  }
  else if (adc_value < 2500U)
  {
    return AIR_LEVEL_NORMAL;
  }
  else
  {
    return AIR_LEVEL_WARNING;
  }
}
