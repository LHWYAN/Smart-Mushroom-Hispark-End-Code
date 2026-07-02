#include "mq2.h"
#include "adc.h"
#include "soc_osal.h"
#include "stdio.h"

/**
 * @brief MQ-2烟雾传感器初始化
 */
int mq2_init(void)
{
    // 初始化 ADC 时钟
    if (uapi_adc_init(ADC_CLOCK_500KHZ) != ERRCODE_SUCC) {
        printf("MQ-2 ADC init failed!\n");
        return -1;
    }
    
    // 打开对应的 ADC 通道
    if (uapi_adc_open_channel(MQ2_ADC_CHANNEL) != ERRCODE_SUCC) {
        printf("MQ-2 ADC open channel %d failed!\n", MQ2_ADC_CHANNEL);
        return -1;
    }
    
    printf("MQ-2 sensor init success, using ADC Channel: %d\n", MQ2_ADC_CHANNEL);
    return 0;
}

/**
 * @brief 获取烟雾传感器的原始ADC平均值
 * @param times 采样次数
 * @return 平均ADC值, 小于0表示出错
 */
int32_t mq2_get_raw_value(uint8_t times)
{
    int32_t sum = 0;
    uint8_t i;
    
    if (times == 0) times = 1;
    
    for (i = 0; i < times; i++) {
        int32_t val = uapi_adc_manual_sample(MQ2_ADC_CHANNEL);
        if (val < 0) {
            return val; // 返回错误码
        }
        sum += val;
        osal_msleep(5); // 短暂延时
    }
    
    return sum / times;
}

/**
 * @brief 获取烟雾浓度百分比值 (0-100)
 * @return 0 - 100 之间的百分比
 */
uint8_t mq2_get_percentage(void)
{
    int32_t raw = mq2_get_raw_value(10); // 采样10次求平均
    uint32_t percentage = 0;
    
    if (raw < 0) {
        return 0; // 出错时返回 0
    }
    
    // 根据原始 STM32 代码逻辑，转换为百分比
    // (raw * 100) / 4000 / 10 => raw / 400
    percentage = raw / 40;
    
    if (percentage > 100) {
        percentage = 100;
    }
    
    return (uint8_t)percentage;
}
