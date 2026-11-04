#include "bh1750.h"

#define BH1750_ADDR_WRITE        0x46U
#define BH1750_ADDR_READ         0x47U
#define BH1750_POWER_ON          0x01U
#define BH1750_RESET             0x07U
#define BH1750_CONT_H_RES_MODE   0x10U

static void IIC_Delay(void)
{
  volatile uint16_t i;
  for (i = 0; i < 80U; i++)
  {
  }
}

/* 软件IIC通信过程中，SDA需要在输出和输入之间切换。 */
static void SDA_Output(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = BH1750_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(BH1750_SDA_PORT, &GPIO_InitStruct);
}

/* 读取ACK或数据时，SDA切换为输入模式。 */
static void SDA_Input(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = BH1750_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BH1750_SDA_PORT, &GPIO_InitStruct);
}

static void IIC_SetSCL(GPIO_PinState state)
{
  HAL_GPIO_WritePin(BH1750_SCL_PORT, BH1750_SCL_PIN, state);
}

static void IIC_SetSDA(GPIO_PinState state)
{
  HAL_GPIO_WritePin(BH1750_SDA_PORT, BH1750_SDA_PIN, state);
}

static GPIO_PinState IIC_ReadSDA(void)
{
  return HAL_GPIO_ReadPin(BH1750_SDA_PORT, BH1750_SDA_PIN);
}

static void IIC_Start(void)
{
  /* IIC起始信号：SCL为高电平时，SDA由高变低。 */
  SDA_Output();
  IIC_SetSDA(GPIO_PIN_SET);
  IIC_SetSCL(GPIO_PIN_SET);
  IIC_Delay();
  IIC_SetSDA(GPIO_PIN_RESET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_RESET);
}

static void IIC_Stop(void)
{
  /* IIC停止信号：SCL为高电平时，SDA由低变高。 */
  SDA_Output();
  IIC_SetSCL(GPIO_PIN_RESET);
  IIC_SetSDA(GPIO_PIN_RESET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_SET);
  IIC_Delay();
  IIC_SetSDA(GPIO_PIN_SET);
  IIC_Delay();
}

static Sensor_Status_t IIC_WaitAck(void)
{
  uint16_t timeout = 200U;

  SDA_Input();
  IIC_SetSDA(GPIO_PIN_SET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_SET);
  IIC_Delay();

  while (IIC_ReadSDA() == GPIO_PIN_SET)
  {
    if (timeout == 0U)
    {
      IIC_Stop();
      return SENSOR_TIMEOUT;
    }
    timeout--;
  }

  IIC_SetSCL(GPIO_PIN_RESET);
  SDA_Output();
  return SENSOR_OK;
}

static void IIC_Ack(void)
{
  /* 主机拉低SDA，表示已经正确接收一个字节。 */
  IIC_SetSCL(GPIO_PIN_RESET);
  SDA_Output();
  IIC_SetSDA(GPIO_PIN_RESET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_SET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_RESET);
}

static void IIC_NAck(void)
{
  /* 最后一个字节后发送NACK，表示本次读取结束。 */
  IIC_SetSCL(GPIO_PIN_RESET);
  SDA_Output();
  IIC_SetSDA(GPIO_PIN_SET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_SET);
  IIC_Delay();
  IIC_SetSCL(GPIO_PIN_RESET);
}

static Sensor_Status_t IIC_SendByte(uint8_t data)
{
  uint8_t i;

  SDA_Output();
  IIC_SetSCL(GPIO_PIN_RESET);

  for (i = 0; i < 8U; i++)
  {
    IIC_SetSDA((data & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    data <<= 1;
    IIC_Delay();
    IIC_SetSCL(GPIO_PIN_SET);
    IIC_Delay();
    IIC_SetSCL(GPIO_PIN_RESET);
    IIC_Delay();
  }

  return IIC_WaitAck();
}

static uint8_t IIC_ReadByte(uint8_t ack)
{
  uint8_t i;
  uint8_t data = 0;

  SDA_Input();
  for (i = 0; i < 8U; i++)
  {
    IIC_SetSCL(GPIO_PIN_RESET);
    IIC_Delay();
    IIC_SetSCL(GPIO_PIN_SET);
    data <<= 1;
    if (IIC_ReadSDA() == GPIO_PIN_SET)
    {
      data++;
    }
    IIC_Delay();
  }

  if (ack)
  {
    IIC_Ack();
  }
  else
  {
    IIC_NAck();
  }

  return data;
}

static Sensor_Status_t BH1750_WriteCommand(uint8_t command)
{
  Sensor_Status_t status;

  IIC_Start();
  status = IIC_SendByte(BH1750_ADDR_WRITE);
  if (status == SENSOR_OK)
  {
    status = IIC_SendByte(command);
  }
  IIC_Stop();

  return status;
}

Sensor_Status_t BH1750_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = BH1750_SCL_PIN | BH1750_SDA_PIN;
  /* 软件IIC使用开漏输出配合上拉，模拟真实IIC总线特性。 */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  IIC_SetSCL(GPIO_PIN_SET);
  IIC_SetSDA(GPIO_PIN_SET);
  HAL_Delay(10);

  if (BH1750_WriteCommand(BH1750_POWER_ON) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }
  HAL_Delay(10);

  (void)BH1750_WriteCommand(BH1750_RESET);
  HAL_Delay(10);

  return BH1750_WriteCommand(BH1750_CONT_H_RES_MODE);
}

Sensor_Status_t BH1750_Read(float *light_lux)
{
  uint8_t high_byte;
  uint8_t low_byte;
  uint16_t raw;

  if (light_lux == 0)
  {
    return SENSOR_ERROR;
  }

  if (BH1750_WriteCommand(BH1750_CONT_H_RES_MODE) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  HAL_Delay(180);

  IIC_Start();
  if (IIC_SendByte(BH1750_ADDR_READ) != SENSOR_OK)
  {
    IIC_Stop();
    return SENSOR_TIMEOUT;
  }

  high_byte = IIC_ReadByte(1U);
  low_byte = IIC_ReadByte(0U);
  IIC_Stop();

  /* BH1750返回两个字节，先合并原始值，再换算为Lux。 */
  raw = ((uint16_t)high_byte << 8) | low_byte;
  *light_lux = (float)raw / 1.2f;

  return SENSOR_OK;
}
