#include "dht11.h"

/*
 * DHT11只使用一根数据线。
 * 在一次通信过程中，这根线需要在输出模式和输入模式之间切换。
 */

static void DHT11_DelayUs(uint32_t us)
{
  uint32_t start;
  uint32_t ticks;

  /* 启用Cortex-M3的DWT计数器，用于实现微秒级延时。 */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  start = DWT->CYCCNT;
  ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
  while ((DWT->CYCCNT - start) < ticks)
  {
  }
}

/*
 * 将PG11配置为输出模式。
 * STM32向DHT11发送起始信号时使用。
 */
static void DHT11_SetOutput(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/*
 * 将PG11配置为输入模式。
 * STM32释放总线并等待DHT11响应或数据时使用。
 */
static void DHT11_SetInput(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/*
 * 等待数据线变为目标电平。
 * 超时保护很重要：如果接线松动，没有超时会导致程序一直卡在等待状态。
 */
static Sensor_Status_t DHT11_WaitPin(GPIO_PinState state, uint32_t timeout_us)
{
  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) != state)
  {
    if (timeout_us == 0U)
    {
      return SENSOR_TIMEOUT;
    }
    timeout_us--;
    DHT11_DelayUs(1);
  }

  return SENSOR_OK;
}

/*
 * 从DHT11读取一个字节。
 * 每一位开始前会有一段低电平，随后通过高电平持续时间表示0或1：
 * 高电平较短表示0，高电平较长表示1。
 */
static Sensor_Status_t DHT11_ReadByte(uint8_t *data)
{
  uint8_t i;
  uint8_t value = 0;

  for (i = 0; i < 8U; i++)
  {
    /* 每一位先等待高电平到来，再根据高电平持续时间判断0/1。 */
    if (DHT11_WaitPin(GPIO_PIN_SET, 100U) != SENSOR_OK)
    {
      return SENSOR_TIMEOUT;
    }

    DHT11_DelayUs(40);
    value <<= 1;
    if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
      value |= 0x01U;
    }

    if (DHT11_WaitPin(GPIO_PIN_RESET, 100U) != SENSOR_OK)
    {
      return SENSOR_TIMEOUT;
    }
  }

  *data = value;
  return SENSOR_OK;
}

Sensor_Status_t DHT11_Init(void)
{
  DHT11_SetOutput();
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
  HAL_Delay(1000);
  return SENSOR_OK;
}

/*
 * 读取DHT11温度和湿度。
 *
 * 通信流程：
 * 1. STM32拉低DATA线，发送起始信号。
 * 2. STM32切换为输入模式，等待DHT11响应。
 * 3. 读取5个字节：湿度整数/小数、温度整数/小数、校验字节。
 * 4. 只有校验正确，才把数据返回给主程序。
 */
Sensor_Status_t DHT11_Read(float *temperature, float *humidity)
{
  uint8_t buffer[5] = {0};
  uint8_t i;
  uint8_t checksum;

  if ((temperature == 0) || (humidity == 0))
  {
    return SENSOR_ERROR;
  }

  /* 起始信号：将数据线拉低至少18ms。 */
  DHT11_SetOutput();
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
  DHT11_DelayUs(30);

  /* 释放总线，切换输入模式，等待DHT11响应。 */
  DHT11_SetInput();
  if (DHT11_WaitPin(GPIO_PIN_RESET, 100U) != SENSOR_OK)
  {
    return SENSOR_TIMEOUT;
  }
  if (DHT11_WaitPin(GPIO_PIN_SET, 100U) != SENSOR_OK)
  {
    return SENSOR_TIMEOUT;
  }
  if (DHT11_WaitPin(GPIO_PIN_RESET, 100U) != SENSOR_OK)
  {
    return SENSOR_TIMEOUT;
  }

  for (i = 0; i < 5U; i++)
  {
    if (DHT11_ReadByte(&buffer[i]) != SENSOR_OK)
    {
      return SENSOR_TIMEOUT;
    }
  }

  checksum = (uint8_t)(buffer[0] + buffer[1] + buffer[2] + buffer[3]);
  if (checksum != buffer[4])
  {
    return SENSOR_DATA_INVALID;
  }

  *humidity = (float)buffer[0] + ((float)buffer[1] / 10.0f);
  *temperature = (float)buffer[2] + ((float)buffer[3] / 10.0f);

  return SENSOR_OK;
}
