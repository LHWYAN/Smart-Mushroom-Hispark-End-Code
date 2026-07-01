/*****************************************************************************************/
/*                                                                                       */
/*                  版权所有：沈阳市网联通信规划设计有限公司                                 */
/*                  开发人员：程国辉 刘艳                                                  */
/*                  联系方式：908536420  3512904489                                       */
/*                  文件名称：ldr.c                                                       */
/*                  功能描述：ADC模拟亮度传感器驱动实现文件                                 */
/*                  开发时间：2025年11月                                                  */
/*                                                                                       */
/*****************************************************************************************/
#include "ldr.h"
#include "pinctrl.h"
#include "common_def.h"
#include "soc_osal.h"

static pin_t LDR;  // 改为static
static int ldr_result = 0;  // 改为int类型
// ADC值转换为百分比函数
uint32_t convert_adc_to_percentage(uint32_t adc_value)
{
    #define ADC_MIN 0   // 最小值
    #define ADC_MAX 3350   // 最大值
    #define PERCENTAGE_RANGE 100  // 百分比范围0-100
    
    // 低于最小值按0处理
    if(adc_value <= ADC_MIN)
        return 0;
    
    // 高于最大值按100处理
    if(adc_value >= ADC_MAX)
        return PERCENTAGE_RANGE;
    
    // 线性映射计算比例值
    // 公式: percentage = (adc_value - ADC_MIN) * 100 / (ADC_MAX - ADC_MIN)
    uint32_t percentage = (adc_value - ADC_MIN) * PERCENTAGE_RANGE / (ADC_MAX - ADC_MIN);
    
    return percentage;
}

//adc采样回调函数
void ldr_callback(uint8_t ch, uint32_t *buffer, uint32_t length, bool *next)
{
    (void)next;

    uint32_t i;
    unsigned long ldr_sum = 0;  // 改为unsigned long
    unsigned long ldr_count = 0;

    // length次采样
    for(i = 0; i < length; i++)
    {
        if(ch == LDR && buffer[i] != 0)
        {
            uint32_t adc_value = buffer[i];
            
            // 检测并处理补码异常值 (如4294964628 = 0xFFFFFCCC = -868)
            if(adc_value > 0x7FFFFFFF)  // 如果最高位为1，说明是补码表示的负数
            {
                // 将补码转换为有符号值，然后取绝对值
                int32_t signed_value = (int32_t)adc_value;  // 转换为有符号
                adc_value = (uint32_t)(-signed_value);      // 取绝对值
                printf("Corrected ADC value: %u (was 0x%08X = %d)\r\n", 
                       adc_value, buffer[i], signed_value);
            }
            
            // 将ADC值转换为0-100的比例值
            uint32_t scaled_value = convert_adc_to_percentage(adc_value);
            
            ldr_sum += scaled_value;
            printf("buffer[%d]=%u -> %u%%\r\n", i, adc_value, scaled_value);
            ldr_count++;
        }
    }
    printf("ldr_sum=%lu ldr_count=%lu\n", ldr_sum, ldr_count);

    if( ldr_count > 0)
        ldr_result = ldr_sum/ldr_count;
    else
        ldr_result = 0;
}

int adc_init(void)
{
    LDR = CONFIG_LDR_ADC_CHANNEL;

    int ret = uapi_adc_init(ADC_CLOCK_500KHZ);
    if( ret != ERRCODE_SUCC )
    {
        printf("uapi_adc_init ADC_CLOCK_500KHZ failed, ret=%d\n", ret);
        return 0;
    }

    uapi_adc_power_en(AFE_SCAN_MODE_MAX_NUM, true);
    return 1;
}

// 关键修改：不要频繁enable/disable ADC
int get_adc_value(void)  // 改为返回int
{
    struct adc_scan_config ldr_config = {
        .type = 0,
        .freq = 1,
    };
    
    // 只在需要时enable，但不要立即disable
    uapi_adc_auto_scan_ch_enable(LDR, ldr_config, ldr_callback);
    osal_msleep(10);
    // 注意：这里移除了disable，或者根据实际需要调整
    //关闭ADC自扫描
    uapi_adc_auto_scan_ch_disable(LDR);
    printf("ldr_result = %d\n", ldr_result);
    return ldr_result;
}