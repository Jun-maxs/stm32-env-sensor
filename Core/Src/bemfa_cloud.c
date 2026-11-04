#include "bemfa_cloud.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

#define ESP8266_RX_BUFFER_SIZE   256U

static Sensor_Status_t ESP8266_SendCommand(const char *cmd, const char *ack, uint32_t timeout_ms);
static Sensor_Status_t ESP8266_WaitFor(const char *ack, uint32_t timeout_ms);
static void ESP8266_ClearRx(void);
static Sensor_Status_t BemfaCloud_OpenTcp(void);
static void BuildFloatText(float value, char *buf, uint8_t len);
static const char *AirLevelText(Air_Level_t level);

/* 保存ESP8266返回内容，调试时也可以在Keil中观察该数组。 */
static uint8_t esp_rx_buffer[ESP8266_RX_BUFFER_SIZE];

Sensor_Status_t BemfaCloud_Init(void)
{
  char cmd[128];

  /*
   * 初始化ESP8266：
   * 1. AT检测模块是否在线；
   * 2. ATE0关闭回显，减少接收内容干扰；
   * 3. CWMODE=1设置为Station模式；
   * 4. CWJAP连接WiFi。
   */
  ESP8266_ClearRx();

  if (ESP8266_SendCommand("AT\r\n", "OK", 1000U) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  if (ESP8266_SendCommand("ATE0\r\n", "OK", 1000U) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  if (ESP8266_SendCommand("AT+CWMODE=1\r\n", "OK", 1000U) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", BEMFA_WIFI_SSID, BEMFA_WIFI_PASSWORD);
  if (ESP8266_SendCommand(cmd, "WIFI GOT IP", 15000U) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  return SENSOR_OK;
}

Sensor_Status_t BemfaCloud_UploadData(const Environment_Data_t *data)
{
  char temp[12];
  char humi[12];
  char light[12];
  char msg[96];
  char payload[180];
  char cmd[32];
  uint16_t length;

  if (data == 0)
  {
    return SENSOR_ERROR;
  }

  BuildFloatText(data->temperature, temp, sizeof(temp));
  BuildFloatText(data->humidity, humi, sizeof(humi));
  BuildFloatText(data->light, light, sizeof(light));

  /*
   * msg字段是上传到巴法云主题中的环境数据。
   * 采用固定字段顺序，方便课堂演示和后续解析。
   */
  snprintf(msg, sizeof(msg),
           "temp=%s,humi=%s,light=%s,air=%u,level=%s",
           data->dht11_valid ? temp : "ERR",
           data->dht11_valid ? humi : "ERR",
           data->bh1750_valid ? light : "ERR",
           data->mq135_valid ? (unsigned int)data->air_quality : 0U,
           data->mq135_valid ? AirLevelText(data->air_level) : "ERR");

  /*
   * 巴法云TCP上传格式：
   * cmd=2&uid=私钥&topic=主题&msg=消息内容
   */
  snprintf(payload, sizeof(payload),
           "cmd=2&uid=%s&topic=%s&msg=%s\r\n",
           BEMFA_UID,
           BEMFA_TOPIC,
           msg);

  if (BemfaCloud_OpenTcp() != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  length = (uint16_t)strlen(payload);
  snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", (unsigned int)length);
  if (ESP8266_SendCommand(cmd, ">", 3000U) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  if (ESP8266_SendCommand(payload, "SEND OK", 5000U) != SENSOR_OK)
  {
    return SENSOR_ERROR;
  }

  (void)ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", 1000U);
  return SENSOR_OK;
}

static Sensor_Status_t BemfaCloud_OpenTcp(void)
{
  char cmd[96];

  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", BEMFA_SERVER, BEMFA_PORT);
  if (ESP8266_SendCommand(cmd, "CONNECT", 5000U) == SENSOR_OK)
  {
    return SENSOR_OK;
  }

  /*
   * 如果模块已经处于TCP连接状态，ESP8266可能返回ALREADY CONNECTED。
   * 这种情况也允许继续发送数据。
   */
  if (strstr((char *)esp_rx_buffer, "ALREADY") != 0)
  {
    return SENSOR_OK;
  }

  return SENSOR_ERROR;
}

static Sensor_Status_t ESP8266_SendCommand(const char *cmd, const char *ack, uint32_t timeout_ms)
{
  if (cmd == 0)
  {
    return SENSOR_ERROR;
  }

  ESP8266_ClearRx();
  HAL_UART_Transmit(&huart3, (uint8_t *)cmd, (uint16_t)strlen(cmd), 1000U);

  if (ack == 0)
  {
    return SENSOR_OK;
  }

  return ESP8266_WaitFor(ack, timeout_ms);
}

static Sensor_Status_t ESP8266_WaitFor(const char *ack, uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  uint16_t index = 0U;
  uint8_t ch;

  memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));

  while ((HAL_GetTick() - start) < timeout_ms)
  {
    if (HAL_UART_Receive(&huart3, &ch, 1U, 10U) == HAL_OK)
    {
      if (index < (ESP8266_RX_BUFFER_SIZE - 1U))
      {
        esp_rx_buffer[index++] = ch;
        esp_rx_buffer[index] = '\0';
      }

      if (strstr((char *)esp_rx_buffer, ack) != 0)
      {
        return SENSOR_OK;
      }

      if (strstr((char *)esp_rx_buffer, "ERROR") != 0)
      {
        return SENSOR_ERROR;
      }
    }
  }

  return SENSOR_TIMEOUT;
}

static void ESP8266_ClearRx(void)
{
  uint8_t ch;

  while (HAL_UART_Receive(&huart3, &ch, 1U, 1U) == HAL_OK)
  {
  }

  memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
}

static void BuildFloatText(float value, char *buf, uint8_t len)
{
  uint32_t integer_part;
  uint32_t decimal_part;

  if ((buf == 0) || (len == 0U))
  {
    return;
  }

  if (value < 0.0f)
  {
    value = 0.0f;
  }

  integer_part = (uint32_t)value;
  decimal_part = (uint32_t)((value - (float)integer_part) * 10.0f + 0.5f);

  if (decimal_part >= 10U)
  {
    integer_part++;
    decimal_part = 0U;
  }

  snprintf(buf, len, "%lu.%lu", (unsigned long)integer_part, (unsigned long)decimal_part);
}

static const char *AirLevelText(Air_Level_t level)
{
  switch (level)
  {
    case AIR_LEVEL_GOOD:
      return "GOOD";
    case AIR_LEVEL_NORMAL:
      return "NORMAL";
    case AIR_LEVEL_WARNING:
      return "WARN";
    default:
      return "UNKNOWN";
  }
}
