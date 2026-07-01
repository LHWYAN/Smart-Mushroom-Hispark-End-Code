/*****************************************************************************************/
/*                                                                                       */
/*                  版权所有：沈阳市网联通信规划设计有限公司                                 */
/*                  开发人员：程国辉 刘艳                                                  */
/*                  联系方式：908536420  3512904489                                       */
/*                  文件名称：buzzer.c                                                    */
/*                  功能描述：蜂鸣器驱动实现文件（适配WS63/Hi3863平台）                     */
/*                  开发时间：2025年11月                                                  */
/*                  本程序只供学习使用，未经作者许可，不得用于其它任何用途                    */
/*                  版本：V1.0                                                           */
/*                  版权所有，盗版必究                                                    */
/*                                                                                       */
/*****************************************************************************************/

#include "buzzer.h"
#include "pinctrl.h"
#include "gpio.h"
#include "stdio.h"

/**
 * @brief 蜂鸣器初始化
 * @return int 成功返回0
 *
 * 注意：蜂鸣器为低电平触发，初始化时设为高电平保持关闭状态
 */
int Buzzer_Init(void)
{
    // 设置IO复用关系，使用普通IO功能
    uapi_pin_set_mode(BUZZER_GPIO_PIN, PIN_MODE_0);

    // 设置IO引脚的方向为输出
    uapi_gpio_set_dir(BUZZER_GPIO_PIN, GPIO_DIRECTION_OUTPUT);

    // 蜂鸣器为低电平触发，初始化时设为高电平，保持关闭状态
    uapi_gpio_set_val(BUZZER_GPIO_PIN, GPIO_LEVEL_HIGH);

    printf("蜂鸣器初始化完成: GPIO_PIN%d\n", BUZZER_GPIO_PIN);

    return 0;
}

/**
 * @brief 蜂鸣器打开 (低电平触发)
 */
void Buzzer_On(void)
{
    uapi_gpio_set_val(BUZZER_GPIO_PIN, GPIO_LEVEL_LOW);
}

/**
 * @brief 蜂鸣器关闭 (高电平)
 */
void Buzzer_Off(void)
{
    uapi_gpio_set_val(BUZZER_GPIO_PIN, GPIO_LEVEL_HIGH);
}

/**
 * @brief 蜂鸣器状态翻转
 */
void Buzzer_Toggle(void)
{
    uapi_gpio_toggle(BUZZER_GPIO_PIN);
}
