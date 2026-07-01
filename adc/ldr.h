/*****************************************************************************************/
/*                                                                                       */
/*                  版权所有：沈阳市网联通信规划设计有限公司                                 */
/*                  开发人员：程国辉 刘艳                                                  */
/*                  联系方式：908536420  3512904489                                       */
/*                  文件名称：ldr.h                                                       */
/*                  功能描述：ADC亮度传感器模拟数据驱动实现头文件                            */
/*                  开发时间：2025年11月                                                  */
/*                  本程序只供学习使用，未经作者许可，不得用于其它任何用途                    */
/*                  版本：V1.0                                                           */
/*                  版权所有，盗版必究                                                    */
/*                                                                                       */
/*****************************************************************************************/
#ifndef _MY_LDR_H_
#define _MY_LDR_H_
#endif

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

int adc_init(void);
void ldr_callback(uint8_t ch, uint32_t *buffer, uint32_t length, bool *next);
int get_adc_value(void);