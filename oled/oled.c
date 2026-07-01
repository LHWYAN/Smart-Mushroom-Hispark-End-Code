
#include "oled.h"

static i2c_bus_t oled_i2c_bus;//定义用来表示IIC总线编号的变量
errcode_t oled_init(void)
{
    oled_i2c_bus = CONFIG_OLED_I2C_BUS;     //IIC总线1

    //使用IIC总线1,设置上拉模式
    uapi_pin_set_mode(GPIO_15, PIN_MODE_2);
    uapi_pin_set_mode(GPIO_16, PIN_MODE_2);
    uapi_pin_set_pull(GPIO_15, PIN_PULL_TYPE_UP);
    uapi_pin_set_pull(GPIO_16, PIN_PULL_TYPE_UP);

    //初始化IIC总线1 ,通信速率为400Khz, 默认配置
    errcode_t ret = uapi_i2c_master_init(oled_i2c_bus, 400000, 0x0);
    if (ret != ERRCODE_SUCC)
    {
        return ret;
    }

    // 初始化OLED模块,代码在bsp_oled.c文件中
    ssd1306_Init(oled_i2c_bus);

    return ERRCODE_SUCC;
}
