#ifndef __BEMFA_CLOUD_H
#define __BEMFA_CLOUD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "env_sensor.h"
#include "sensor_status.h"

/*
 * 巴法云配置区。
 *
 * ESP8266使用USART3与STM32通信：
 * USART3_TX: PB10
 * USART3_RX: PB11
 *
 * 注意：STM32_TX需要接ESP8266_RX，STM32_RX需要接ESP8266_TX。
 */
#define BEMFA_WIFI_SSID        "YOUR_WIFI_SSID"
#define BEMFA_WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"
#define BEMFA_UID              "YOUR_BEMFA_UID"
#define BEMFA_TOPIC            "sensor004"

#define BEMFA_SERVER           "bemfa.com"
#define BEMFA_PORT             8344

Sensor_Status_t BemfaCloud_Init(void);
Sensor_Status_t BemfaCloud_UploadData(const Environment_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif
