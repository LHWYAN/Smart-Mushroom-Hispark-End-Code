#ifndef _MY_DHT11_H_
#define _MY_DHT11_H_

#include "pinctrl.h"
#include "common_def.h"
#include "soc_osal.h"
#include "osal_wait.h"
#include "app_init.h"
#include "gpio.h"
#include "adc.h"
#include "adc_porting.h"
#include "stdio.h"
#include "hal_gpio.h"
#include "hal_timer.h"

// DHT11 数据类型定义
typedef struct
{
	uint8_t humi_high8bit; // 原始数据：湿度高8位
	uint8_t humi_low8bit;  // 原始数据：湿度低8位
	uint8_t temp_high8bit; // 原始数据：温度高8位
	uint8_t temp_low8bit;  // 原始数据：温度高8位
	uint8_t check_sum;	   // 校验和
	float humidity;		   // 实际湿度
	float temperature;	   // 实际温度
} DHT11_Data_TypeDef;


//DHT11传感器初始化
void dht11_init(void);
errcode_t dht11_read_data(DHT11_Data_TypeDef *DHT11_Data);

#endif
