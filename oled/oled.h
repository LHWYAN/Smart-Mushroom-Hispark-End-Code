#ifndef OLED_H
#define OLED_H

#endif

#define CONFIG_OLED_I2C_BUS 1
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"

#include "oled_fonts.h"
#include "bsp_oled.h"
#include "pinctrl.h"
#include "gpio.h"

errcode_t oled_init(void);