/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "env_sensor.h"
#include "oled_8080.h"
#include "bemfa_cloud.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BEMFA_UPLOAD_PERIOD_MS   10000U
#define BEMFA_RETRY_PERIOD_MS    60000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/*
 * env_data是整个系统统一使用的环境数据容器。
 * 传感器采集、OLED显示、串口输出和巴法云上传都读取这一份数据。
 */
static Environment_Data_t env_data;

/* 巴法云上传状态变量，用于控制上传周期和失败后的重连周期。 */
static uint32_t bemfa_last_upload_tick = 0U;
static uint32_t bemfa_last_retry_tick = 0U;
static uint8_t bemfa_cloud_ready = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* 不开启printf浮点支持时，用这个函数输出一位小数。 */
static void Print_Float1(const char *name, float value, const char *unit);
static const char *AirLevel_ToString(Air_Level_t level);
static void Print_Environment_Data(const Environment_Data_t *data);
static void BemfaCloud_Task(const Environment_Data_t *data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  /* USART1作为课堂演示和调试输出串口。 */
  printf("\r\n");
  printf("========================================\r\n");
  printf(" Environment Sensor System\r\n");
  printf(" DHT11:  PG11\r\n");
  printf(" BH1750: PB6(SCL), PB7(SDA), software IIC\r\n");
  printf(" MQ135:  PA1 / ADC1_IN1\r\n");
  printf(" OLED:   8080 parallel interface\r\n");
  printf(" USART1: debug output, 115200 8N1\r\n");
  printf(" USART3: ESP8266 WiFi module, 115200 8N1\r\n");
  printf("========================================\r\n");

  OLED8080_Init();
  OLED8080_ShowString(0, 0, "Env Sensor");
  OLED8080_ShowString(0, 2, "OLED Ready");

  /*
   * 通过统一接口初始化所有传感器模块。
   * 即使某个模块初始化失败，程序也继续运行，便于通过串口定位问题。
   */
  if (EnvSensor_Init(&env_data) == SENSOR_OK)
  {
    printf("Sensor init: OK\r\n");
  }
  else
  {
    printf("Sensor init: some module failed, continue testing...\r\n");
  }

  /*
   * 初始化巴法云上传模块。
   * 如果WiFi连接失败，系统仍然继续本地采集、OLED显示和串口输出。
   */
  OLED8080_ShowString(0, 4, "WiFi Init...");
  if (BemfaCloud_Init() == SENSOR_OK)
  {
    bemfa_cloud_ready = 1U;
    bemfa_last_upload_tick = HAL_GetTick();
    printf("Bemfa cloud init: OK\r\n");
    OLED8080_ShowString(0, 6, "WiFi OK");
  }
  else
  {
    bemfa_cloud_ready = 0U;
    bemfa_last_retry_tick = HAL_GetTick();
    printf("Bemfa cloud init: failed, continue local display...\r\n");
    OLED8080_ShowString(0, 6, "WiFi Failed");
  }
  HAL_Delay(1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /*
     * 主循环任务：
     * 1. 统一读取传感器；
     * 2. 刷新OLED本地显示；
     * 3. 通过USART1输出调试信息；
     * 4. 按周期上传到巴法云。
     */
    EnvSensor_ReadAll(&env_data);
    OLED8080_ShowEnvData(&env_data);
    Print_Environment_Data(&env_data);
    BemfaCloud_Task(&env_data);

    /* DHT11不适合过快读取，2秒周期适合课堂测试。 */
    HAL_Delay(2000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/*
 * 将printf()重定向到USART1。
 * Keil中printf输出的每个字符都会调用fputc()。
 */
int fputc(int ch, FILE *f)
{
  (void)f;
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/*
 * 输出一个正数浮点值，保留一位小数。
 * 这样可以避免在Keil中额外开启printf浮点输出选项。
 */
static void Print_Float1(const char *name, float value, const char *unit)
{
  uint32_t integer_part;
  uint32_t decimal_part;

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

  printf("%s: %lu.%lu %s\r\n",
         name,
         (unsigned long)integer_part,
         (unsigned long)decimal_part,
         unit);
}

/* 将空气质量枚举值转换为便于串口显示的字符串。 */
static const char *AirLevel_ToString(Air_Level_t level)
{
  switch (level)
  {
    case AIR_LEVEL_GOOD:
      return "GOOD";
    case AIR_LEVEL_NORMAL:
      return "NORMAL";
    case AIR_LEVEL_WARNING:
      return "WARNING";
    default:
      return "UNKNOWN";
  }
}

/* 通过USART1输出当前环境数据，便于课堂观察和调试。 */
static void Print_Environment_Data(const Environment_Data_t *data)
{
  if (data == 0)
  {
    return;
  }

  printf("\r\n---- Environment Data ----\r\n");

  if (data->dht11_valid)
  {
    Print_Float1("Temperature", data->temperature, "C");
    Print_Float1("Humidity", data->humidity, "%RH");
  }
  else
  {
    printf("DHT11: read failed\r\n");
  }

  if (data->bh1750_valid)
  {
    Print_Float1("Light", data->light, "Lux");
  }
  else
  {
    printf("BH1750: read failed\r\n");
  }

  if (data->mq135_valid)
  {
    printf("Air ADC: %u, Level: %s\r\n",
           (unsigned int)data->air_quality,
           AirLevel_ToString(data->air_level));
  }
  else
  {
    printf("MQ135: read failed\r\n");
  }
}

/*
 * 巴法云上传任务。
 * 网络通信可能出现超时，因此上传周期要比OLED刷新周期更长。
 */
static void BemfaCloud_Task(const Environment_Data_t *data)
{
  uint32_t now = HAL_GetTick();

  if ((bemfa_cloud_ready != 0U) && ((now - bemfa_last_upload_tick) >= BEMFA_UPLOAD_PERIOD_MS))
  {
    bemfa_last_upload_tick = now;
    if (BemfaCloud_UploadData(data) == SENSOR_OK)
    {
      printf("Bemfa upload: OK\r\n");
    }
    else
    {
      bemfa_cloud_ready = 0U;
      bemfa_last_retry_tick = now;
      printf("Bemfa upload: failed\r\n");
    }
  }
  else if ((bemfa_cloud_ready == 0U) && ((now - bemfa_last_retry_tick) >= BEMFA_RETRY_PERIOD_MS))
  {
    bemfa_last_retry_tick = now;
    printf("Bemfa reconnect...\r\n");
    if (BemfaCloud_Init() == SENSOR_OK)
    {
      bemfa_cloud_ready = 1U;
      bemfa_last_upload_tick = HAL_GetTick();
      printf("Bemfa reconnect: OK\r\n");
    }
    else
    {
      printf("Bemfa reconnect: failed\r\n");
    }
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
