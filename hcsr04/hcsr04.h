/*****************************************************************************************/
/*                                                                                       */
/*                  版权所有：沈阳市网联通信规划设计有限公司                                 */
/*                  开发人员：程国辉 刘艳                                                  */
/*                  联系方式：908536420  3512904489                                       */
/*                  文件名称：hcsr04.c                                                    */
/*                  功能描述：HCSR04传感器驱动头文件                                     */
/*                  开发时间：2025年11月                                                  */
/*                  本程序只供学习使用，未经作者许可，不得用于其它任何用途                    */
/*                  版本：V1.0                                                           */
/*                  版权所有，盗版必究                                                    */
/*                                                                                       */
/*****************************************************************************************/

#ifndef HCSR04_H
#define HCSR04_H

#include "stdint.h"
#include "gpio.h"  // 包含GPIO相关宏定义（如GPIO_LEVEL_HIGH等）

// 函数返回值定义
#define HCSR04_SUCCESS       0   // 测量成功
#define HCSR04_ERR_TIMEOUT   -1  // 超时错误（未检测到回声）
#define HCSR04_ERR_INVALID   -2  // 无效参数

/**
 * @brief 初始化HCSR04传感器
 * @param trig_pin TRIG引脚编号（如GPIO_06）
 * @param echo_pin ECHO引脚编号（如GPIO_09）
 * @return 0-成功，-2-无效参数
 */
void hcsr04_init(void);

/**
 * @brief 获取HCSR04测量的距离
 * @param distance 输出参数，存储测量到的距离（单位：cm）
 * @param max_distance 最大测量距离（单位：cm，超过此值视为超时）
 * @return 0-成功，-1-超时，-2-未初始化
 */
int32_t hcsr04_get_distance(void);

#endif  