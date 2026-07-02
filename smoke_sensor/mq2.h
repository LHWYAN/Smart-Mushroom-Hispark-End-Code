#ifndef __MQ2_H__
#define __MQ2_H__

#include <stdint.h>
#include "pinctrl.h"

/* 可根据实际硬件连接修改MQ-2传感器的ADC引脚号，此处默认使用ADC通道 2 */
#ifndef MQ2_ADC_CHANNEL
#define MQ2_ADC_CHANNEL 2
#endif

/* 烟雾传感器初始化 */
int mq2_init(void);

/* 获取烟雾传感器的原始ADC平均值 */
int32_t mq2_get_raw_value(uint8_t times);

/* 获取烟雾浓度百分比值 (0-100) */
uint8_t mq2_get_percentage(void);

#endif // __MQ2_H__
