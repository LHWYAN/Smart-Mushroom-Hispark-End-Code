/*****************************************************************************************/
/*                                                                                       */
/*                  版权所有：沈阳市网联通信规划设计有限公司                                 */
/*                  开发人员：程国辉 刘艳                                                  */
/*                  联系方式：908536420  3512904489                                       */
/*                  文件名称：hcsr04.c                                                    */
/*                  功能描述：HCSR04传感器驱动实现文件                                     */
/*                  开发时间：2025年11月                                                  */
/*                  本程序只供学习使用，未经作者许可，不得用于其它任何用途                    */
/*                  版本：V1.0                                                           */
/*                  版权所有，盗版必究                                                    */
/*                                                                                       */
/*****************************************************************************************/

#include "pinctrl.h"
#include "common_def.h"
#include "soc_osal.h"
#include "osal_wait.h"
#include "app_init.h"
#include "gpio.h"
#include "stdio.h"
#include "timer.h"

// 超声波引脚定义（全局可用）
#define TRIG_PIN    GPIO_06
#define ECHO_PIN    GPIO_09

// 最大测量距离(单位:cm)，超过此值视为无效
#define MAX_DISTANCE    800
// 超时时间(单位:us)，根据最大距离计算得出
#define TIMEOUT_US      (MAX_DISTANCE * 58)  // 1cm约对应58us

// 微秒级延时函数(核心：纯软件实现，不依赖系统接口)
void hcsr04_udelay(uint16_t us)
{
    for (volatile uint16_t i = 0; i < us; i++)
    {
        volatile uint16_t j = 30;  // 可根据实际硬件调整延时精度
        while (j--)
            ;
    }
}

// 初始化函数：配置TRIG和ECHO引脚
void hcsr04_init(void) {
    // 配置TRIG为输出
    uapi_pin_set_mode(TRIG_PIN, PIN_MODE_0);
    uapi_gpio_set_dir(TRIG_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(TRIG_PIN, GPIO_LEVEL_LOW);

    // 配置ECHO为输入
    uapi_pin_set_mode(ECHO_PIN, PIN_MODE_0);
    uapi_gpio_set_dir(ECHO_PIN, GPIO_DIRECTION_INPUT);
}

// 获取距离函数：纯软件计时，不依赖osal_get_us()
int32_t hcsr04_get_distance(void) {
    uint64_t start_accumulate = 0;  // 计时累加器（us）
    uint64_t end_accumulate = 0;
    uint32_t duration;
    uint8_t echo_state;

    // 1. 发送10us高电平触发信号
    uapi_gpio_set_val(TRIG_PIN, GPIO_LEVEL_HIGH);
    hcsr04_udelay(10);  // 用自定义延时，不依赖系统osal_udelay
    uapi_gpio_set_val(TRIG_PIN, GPIO_LEVEL_LOW);

    // 2. 等待ECHO引脚变为高电平（超时检测）
    start_accumulate = 0;
    do {
        echo_state = uapi_gpio_get_val(ECHO_PIN);
        hcsr04_udelay(10);  // 每次检查延时10us，平衡精度和CPU占用
        start_accumulate += 10;  // 累加计时
        if (start_accumulate > TIMEOUT_US) {
            return -1;  // 超时：未检测到回声
        }
    } while (echo_state != GPIO_LEVEL_HIGH);

    // 3. 记录ECHO高电平开始时间
    end_accumulate = start_accumulate;

    // 4. 等待ECHO引脚变为低电平（超时检测）
    do {
        echo_state = uapi_gpio_get_val(ECHO_PIN);
        hcsr04_udelay(10);
        end_accumulate += 10;  // 继续累加计时
        if (end_accumulate - start_accumulate > TIMEOUT_US) {
            return -1;  // 超时：超过最大测量距离
        }
    } while (echo_state != GPIO_LEVEL_LOW);

    // 5. 计算距离（1cm ≈ 58us）
    duration = end_accumulate - start_accumulate;
    return duration / 58;
}
