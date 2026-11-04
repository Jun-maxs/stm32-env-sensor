#ifndef __SENSOR_STATUS_H
#define __SENSOR_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  /* 操作成功，输出数据有效。 */
  SENSOR_OK = 0,
  /* 通用错误，例如空指针或外设操作失败。 */
  SENSOR_ERROR,
  /* 在规定时间内没有等到传感器响应。 */
  SENSOR_TIMEOUT,
  /* 已收到数据，但校验或范围判断失败。 */
  SENSOR_DATA_INVALID
} Sensor_Status_t;

#ifdef __cplusplus
}
#endif

#endif
