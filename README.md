# STM32 智能环境监测系统

基于 STM32F103ZET6 的环境监测工程，用 STM32CubeMX 生成初始化代码，Keil MDK5 编译，VSCode + Keil Assistant 编辑与构建，Git 管理版本。

## 硬件接口

| 模块 | 接口 |
| --- | --- |
| DHT11 温湿度 | PG11 单总线 |
| BH1750 光照 | PB6(SCL) / PB7(SDA)，软件 I2C |
| MQ135 空气质量 | PA1 / ADC1_IN1 |
| OLED 0.96 寸 | 8080 并口（SSD1306） |
| 调试串口 | USART1，115200 8N1 |
| ESP8266 WiFi | USART3，115200 8N1，接巴法云 |

## 目录

- `Core/`：应用代码与外设驱动（main.c、dht11.c、bh1750.c、mq135.c、oled_8080.c、bemfa_cloud.c）
- `Drivers/`：CMSIS 与 STM32F1 HAL 库
- `MDK-ARM/`：Keil 工程文件 `Environment_Sensor.uvprojx`
- `Environment_Sensor.ioc`：CubeMX 配置

## 编译

VSCode 安装 Keil Assistant 插件，设置 `KeilAssistant.MDK.Uv4Path` 为 `C:\Keil_v5\UV4\UV4.exe`，在 KEIL UVISION PROJECT 视图中打开 `MDK-ARM/Environment_Sensor.uvprojx`，点击 Build 即可生成 hex 文件。

## 版本记录

| 版本 | 说明 |
| --- | --- |
| v1.0.0 | 导入课程案例工程，传感器采集、OLED 显示、巴法云上报 |
| v1.1.0 | 串口启动信息打印固件版本号；OLED 开机画面显示版本号（feature/oled-version 分支合并） |
