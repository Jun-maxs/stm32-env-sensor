#include "oled_8080.h"
#include <stdio.h>

#define OLED_DATA_PORT      GPIOC
#define OLED_CS_PORT        GPIOD
#define OLED_CS_PIN         GPIO_PIN_6
#define OLED_RS_PORT        GPIOD
#define OLED_RS_PIN         GPIO_PIN_3
#define OLED_WR_PORT        GPIOG
#define OLED_WR_PIN         GPIO_PIN_14
#define OLED_RD_PORT        GPIOG
#define OLED_RD_PIN         GPIO_PIN_13
#define OLED_RST_PORT       GPIOG
#define OLED_RST_PIN        GPIO_PIN_15

#define OLED_CMD            0U
#define OLED_DATA           1U

static void OLED8080_WriteByte(uint8_t byte, uint8_t type);
static void OLED8080_SetPos(uint8_t x, uint8_t page);
static void OLED8080_ShowChar(uint8_t x, uint8_t page, char ch);
static const uint8_t *OLED8080_GetGlyph(char ch);
static void FloatToText1(float value, char *buf, uint8_t len);
static const char *AirLevelText(Air_Level_t level);

/*
 * 简化ASCII字模，每个字符宽5像素、高8像素，显示时额外补1列空白。
 * 本项目只需要显示英文、数字和少量符号，因此不引入完整字库，便于学生理解。
 */
static const uint8_t glyph_space[5] = {0x00,0x00,0x00,0x00,0x00};
static const uint8_t glyph_colon[5] = {0x00,0x36,0x36,0x00,0x00};
static const uint8_t glyph_dot[5]   = {0x00,0x60,0x60,0x00,0x00};
static const uint8_t glyph_pct[5]   = {0x62,0x64,0x08,0x13,0x23};
static const uint8_t glyph_dash[5]  = {0x08,0x08,0x08,0x08,0x08};
static const uint8_t glyph_0[5] = {0x3E,0x51,0x49,0x45,0x3E};
static const uint8_t glyph_1[5] = {0x00,0x42,0x7F,0x40,0x00};
static const uint8_t glyph_2[5] = {0x42,0x61,0x51,0x49,0x46};
static const uint8_t glyph_3[5] = {0x21,0x41,0x45,0x4B,0x31};
static const uint8_t glyph_4[5] = {0x18,0x14,0x12,0x7F,0x10};
static const uint8_t glyph_5[5] = {0x27,0x45,0x45,0x45,0x39};
static const uint8_t glyph_6[5] = {0x3C,0x4A,0x49,0x49,0x30};
static const uint8_t glyph_7[5] = {0x01,0x71,0x09,0x05,0x03};
static const uint8_t glyph_8[5] = {0x36,0x49,0x49,0x49,0x36};
static const uint8_t glyph_9[5] = {0x06,0x49,0x49,0x29,0x1E};
static const uint8_t glyph_A[5] = {0x7E,0x11,0x11,0x11,0x7E};
static const uint8_t glyph_C[5] = {0x3E,0x41,0x41,0x41,0x22};
static const uint8_t glyph_D[5] = {0x7F,0x41,0x41,0x22,0x1C};
static const uint8_t glyph_E[5] = {0x7F,0x49,0x49,0x49,0x41};
static const uint8_t glyph_F[5] = {0x7F,0x09,0x09,0x09,0x01};
static const uint8_t glyph_G[5] = {0x3E,0x41,0x49,0x49,0x7A};
static const uint8_t glyph_H[5] = {0x7F,0x08,0x08,0x08,0x7F};
static const uint8_t glyph_I[5] = {0x00,0x41,0x7F,0x41,0x00};
static const uint8_t glyph_L[5] = {0x7F,0x40,0x40,0x40,0x40};
static const uint8_t glyph_M[5] = {0x7F,0x02,0x0C,0x02,0x7F};
static const uint8_t glyph_N[5] = {0x7F,0x04,0x08,0x10,0x7F};
static const uint8_t glyph_O[5] = {0x3E,0x41,0x41,0x41,0x3E};
static const uint8_t glyph_R[5] = {0x7F,0x09,0x19,0x29,0x46};
static const uint8_t glyph_S[5] = {0x46,0x49,0x49,0x49,0x31};
static const uint8_t glyph_T[5] = {0x01,0x01,0x7F,0x01,0x01};
static const uint8_t glyph_U[5] = {0x3F,0x40,0x40,0x40,0x3F};
static const uint8_t glyph_W[5] = {0x3F,0x40,0x38,0x40,0x3F};
static const uint8_t glyph_a[5] = {0x20,0x54,0x54,0x54,0x78};
static const uint8_t glyph_d[5] = {0x38,0x44,0x44,0x48,0x7F};
static const uint8_t glyph_e[5] = {0x38,0x54,0x54,0x54,0x18};
static const uint8_t glyph_g[5] = {0x0C,0x52,0x52,0x52,0x3E};
static const uint8_t glyph_h[5] = {0x7F,0x08,0x04,0x04,0x78};
static const uint8_t glyph_i[5] = {0x00,0x44,0x7D,0x40,0x00};
static const uint8_t glyph_l[5] = {0x00,0x41,0x7F,0x40,0x00};
static const uint8_t glyph_m[5] = {0x7C,0x04,0x18,0x04,0x78};
static const uint8_t glyph_n[5] = {0x7C,0x08,0x04,0x04,0x78};
static const uint8_t glyph_o[5] = {0x38,0x44,0x44,0x44,0x38};
static const uint8_t glyph_p[5] = {0x7C,0x14,0x14,0x14,0x08};
static const uint8_t glyph_r[5] = {0x7C,0x08,0x04,0x04,0x08};
static const uint8_t glyph_s[5] = {0x48,0x54,0x54,0x54,0x20};
static const uint8_t glyph_t[5] = {0x04,0x3F,0x44,0x40,0x20};
static const uint8_t glyph_u[5] = {0x3C,0x40,0x40,0x20,0x7C};
static const uint8_t glyph_v[5] = {0x1C,0x20,0x40,0x20,0x1C};
static const uint8_t glyph_x[5] = {0x44,0x28,0x10,0x28,0x44};
static const uint8_t glyph_y[5] = {0x0C,0x50,0x50,0x50,0x3C};

void OLED8080_Init(void)
{
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(OLED_RD_PORT, OLED_RD_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(50);

  OLED8080_WriteByte(0xAE, OLED_CMD);
  OLED8080_WriteByte(0x00, OLED_CMD);
  OLED8080_WriteByte(0x10, OLED_CMD);
  OLED8080_WriteByte(0x40, OLED_CMD);
  OLED8080_WriteByte(0x81, OLED_CMD);
  OLED8080_WriteByte(0xCF, OLED_CMD);
  OLED8080_WriteByte(0xA1, OLED_CMD);
  OLED8080_WriteByte(0xC8, OLED_CMD);
  OLED8080_WriteByte(0xA6, OLED_CMD);
  OLED8080_WriteByte(0xA8, OLED_CMD);
  OLED8080_WriteByte(0x3F, OLED_CMD);
  OLED8080_WriteByte(0xD3, OLED_CMD);
  OLED8080_WriteByte(0x00, OLED_CMD);
  OLED8080_WriteByte(0xD5, OLED_CMD);
  OLED8080_WriteByte(0x80, OLED_CMD);
  OLED8080_WriteByte(0xD9, OLED_CMD);
  OLED8080_WriteByte(0xF1, OLED_CMD);
  OLED8080_WriteByte(0xDA, OLED_CMD);
  OLED8080_WriteByte(0x12, OLED_CMD);
  OLED8080_WriteByte(0xDB, OLED_CMD);
  OLED8080_WriteByte(0x40, OLED_CMD);
  OLED8080_WriteByte(0x8D, OLED_CMD);
  OLED8080_WriteByte(0x14, OLED_CMD);
  OLED8080_WriteByte(0xAF, OLED_CMD);

  OLED8080_Clear();
}

void OLED8080_Clear(void)
{
  uint8_t page;
  uint8_t x;

  for (page = 0; page < 8; page++)
  {
    OLED8080_SetPos(0, page);
    for (x = 0; x < 128; x++)
    {
      OLED8080_WriteByte(0x00, OLED_DATA);
    }
  }
}

void OLED8080_ShowString(uint8_t x, uint8_t page, const char *str)
{
  while ((str != 0) && (*str != '\0') && (x <= 122U) && (page < 8U))
  {
    OLED8080_ShowChar(x, page, *str);
    x += 6U;
    str++;
  }
}

void OLED8080_ShowEnvData(const Environment_Data_t *data)
{
  char line[22];
  char text[10];

  if (data == 0)
  {
    return;
  }

  OLED8080_Clear();

  if (data->dht11_valid)
  {
    FloatToText1(data->temperature, text, sizeof(text));
    snprintf(line, sizeof(line), "Temp:%sC", text);
    OLED8080_ShowString(0, 0, line);

    FloatToText1(data->humidity, text, sizeof(text));
    snprintf(line, sizeof(line), "Humi:%s%%", text);
    OLED8080_ShowString(0, 2, line);
  }
  else
  {
    OLED8080_ShowString(0, 0, "Temp:ERROR");
    OLED8080_ShowString(0, 2, "Humi:ERROR");
  }

  if (data->bh1750_valid)
  {
    FloatToText1(data->light, text, sizeof(text));
    snprintf(line, sizeof(line), "Light:%sLux", text);
    OLED8080_ShowString(0, 4, line);
  }
  else
  {
    OLED8080_ShowString(0, 4, "Light:ERROR");
  }

  if (data->mq135_valid)
  {
    snprintf(line, sizeof(line), "Air:%u %s",
             (unsigned int)data->air_quality,
             AirLevelText(data->air_level));
    OLED8080_ShowString(0, 6, line);
  }
  else
  {
    OLED8080_ShowString(0, 6, "Air:ERROR");
  }
}

static void OLED8080_WriteByte(uint8_t byte, uint8_t type)
{
  /*
   * 8080写时序：
   * 1. RS选择命令或数据；
   * 2. PC0-PC7输出8位数据；
   * 3. WR产生一个低脉冲，把数据写入OLED。
   */
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN, (type == OLED_DATA) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_RD_PORT, OLED_RD_PIN, GPIO_PIN_SET);

  OLED_DATA_PORT->ODR = (OLED_DATA_PORT->ODR & 0xFF00U) | byte;

  HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_RESET);
  __NOP();
  HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}

static void OLED8080_SetPos(uint8_t x, uint8_t page)
{
  OLED8080_WriteByte((uint8_t)(0xB0U + page), OLED_CMD);
  OLED8080_WriteByte((uint8_t)(((x & 0xF0U) >> 4U) | 0x10U), OLED_CMD);
  OLED8080_WriteByte((uint8_t)(x & 0x0FU), OLED_CMD);
}

static void OLED8080_ShowChar(uint8_t x, uint8_t page, char ch)
{
  const uint8_t *glyph = OLED8080_GetGlyph(ch);
  uint8_t i;

  OLED8080_SetPos(x, page);
  for (i = 0; i < 5; i++)
  {
    OLED8080_WriteByte(glyph[i], OLED_DATA);
  }
  OLED8080_WriteByte(0x00, OLED_DATA);
}

static const uint8_t *OLED8080_GetGlyph(char ch)
{
  switch (ch)
  {
    case ' ': return glyph_space;
    case ':': return glyph_colon;
    case '.': return glyph_dot;
    case '%': return glyph_pct;
    case '-': return glyph_dash;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case '9': return glyph_9;
    case 'A': return glyph_A;
    case 'C': return glyph_C;
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'I': return glyph_I;
    case 'L': return glyph_L;
    case 'M': return glyph_M;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'R': return glyph_R;
    case 'S': return glyph_S;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    case 'W': return glyph_W;
    case 'a': return glyph_a;
    case 'd': return glyph_d;
    case 'e': return glyph_e;
    case 'g': return glyph_g;
    case 'h': return glyph_h;
    case 'i': return glyph_i;
    case 'l': return glyph_l;
    case 'm': return glyph_m;
    case 'n': return glyph_n;
    case 'o': return glyph_o;
    case 'p': return glyph_p;
    case 'r': return glyph_r;
    case 's': return glyph_s;
    case 't': return glyph_t;
    case 'u': return glyph_u;
    case 'v': return glyph_v;
    case 'x': return glyph_x;
    case 'y': return glyph_y;
    default:  return glyph_space;
  }
}

static void FloatToText1(float value, char *buf, uint8_t len)
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
      return "ERROR";
  }
}
