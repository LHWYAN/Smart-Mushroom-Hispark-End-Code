/*****************************************************************************************/
/*                                                                                       */
/*                  版权所有：沈阳市网联通信规划设计有限公司                                 */
/*                  开发人员：程国辉 刘艳                                                  */
/*                  联系方式：908536420  3512904489                                        */
/*                  程序主题：TCP+WIFi+MQTT连接华为云实验                                   */
/*                  开发时间：2025年11月                                                  */
/*                  本程序只供学习使用，未经作者许可，不得用于其它任何用途                    */
/*                  版本：V1.0                                                           */
/*                  版权所有，盗版必究                                                    */
/*                                                                                       */
/*****************************************************************************************/

#include "lwip/netifapi.h"
#include "wifi_hotspot.h"
#include "wifi_hotspot_config.h"
#include "stdlib.h"
#include "uart.h"
#include "lwip/nettool/misc.h"
#include "soc_osal.h"
#include "app_init.h"
#include "cmsis_os2.h"
#include "wifi_device.h"
#include "wifi_event.h"
#include "lwip/sockets.h"
#include "lwip/ip4_addr.h"
#include "wifi/wifi_connect.h"
#include "dht11/dht11.h"
#include "oled/oled.h"
#include "app_main.h"
#include "adc/ldr.h"
#include "led/led.h"
#include "buzzer/buzzer.h"
#include "smoke_sensor/mq2.h"


#define WIFI_TASK_STACK_SIZE 0x2000

#define DELAY_TIME_MS 100

DHT11_Data_TypeDef DHT11_Data;  //存放温度数据

char LampSt[4] = {0};//灯状态
int lampState=1;

char CondiSt[4] = {0};//空调状态
int condiState = 0;

char VentSt[4] = {0};//排风扇状态
int ventState = 0;

char BuzzerSt[4] = {0};//蜂鸣器状态
int buzzerState = 0;

char SmokeSt[16] = {0};//烟雾传感器
char CO2St[16] = {0};//二氧化碳浓度

uint16_t ldr_value;


//以下为采集环境信息的子任务，源源不断的采集各种物联网环境信息数据
static void *environment_task(const char *arg)
{
     unused(arg);

     char lcd_buff[100]={0};
     errcode_t result;
     osal_msleep(1000);  //先稳定一下情绪每秒钟采集一次信息

     while(1)
     {

        result = dht11_read_data(&DHT11_Data);
         if(result ==  ERRCODE_SUCC)
         {
            printf("Temperature:%d.%d, Humidity:%d.%d\n", DHT11_Data.temp_high8bit, DHT11_Data.temp_low8bit, DHT11_Data.humi_high8bit, DHT11_Data.humi_low8bit);
            memset(lcd_buff,0,100);
            sprintf(lcd_buff, "Temp:%d.%d " ,DHT11_Data.temp_high8bit,DHT11_Data.temp_low8bit);
            bsp_oled_DrawString(0, 0, lcd_buff, Font_7x10, White);
            memset(lcd_buff,0,100);
            sprintf(lcd_buff, "Humi:%d.%d " ,DHT11_Data.humi_high8bit,DHT11_Data.humi_low8bit);
            bsp_oled_DrawString(0, 10, lcd_buff, Font_7x10, White);
            bsp_oled_UpdateScreen();
        }
        else{
            printf("Read DHT11 data fail.\n");
         }

        ldr_value = get_adc_value();
        memset(lcd_buff,0,100);
        sprintf(lcd_buff, "Lumi:%d   " ,ldr_value);
        bsp_oled_DrawString(0, 20, lcd_buff, Font_7x10, White);
        if (ldr_value > 50) {
             bsp_oled_DrawString(0, 40, "LampSt:ON ", Font_7x10, White);
             led_on(1);
             lampState=0; //设置灯泡状态为开
        } else {
             bsp_oled_DrawString(0, 40, "LampSt:OFF", Font_7x10, White);
             led_off(1);
             lampState=1; //设置灯泡状态为关
        }


        memset(lcd_buff,0,100);
        if (condiState == 1) {
            sprintf(lcd_buff, "CondST:ON  ");
        } else {
            sprintf(lcd_buff, "CondST:OFF ");
        }
        bsp_oled_DrawString(0, 30, lcd_buff, Font_7x10, White);

        memset(lcd_buff,0,100);
        if (ventState == 1) {
            sprintf(lcd_buff, "VentST:ON  ");
        } else {
            sprintf(lcd_buff, "VentST:OFF ");
        }
        bsp_oled_DrawString(0, 50, lcd_buff, Font_7x10, White);

        /* 读取MQ-2烟雾传感器 */
        uint8_t smoke_val = mq2_get_percentage();
        snprintf_s(SmokeSt, sizeof(SmokeSt), sizeof(SmokeSt) - 1, "%d", smoke_val);
        memset(lcd_buff,0,100);
        sprintf(lcd_buff, "Smoke:%d%%   ", smoke_val);
        bsp_oled_DrawString(0, 60, lcd_buff, Font_7x10, White);

        if (lampState == 0) {
            snprintf_s(LampSt, sizeof(LampSt), sizeof(LampSt) - 1, "ON");
        } else {
            snprintf_s(LampSt, sizeof(LampSt), sizeof(LampSt) - 1, "OFF");
        }

        if (condiState == 1) {
            snprintf_s(CondiSt, sizeof(CondiSt), sizeof(CondiSt) - 1, "ON");
        } else {
            snprintf_s(CondiSt, sizeof(CondiSt), sizeof(CondiSt) - 1, "OFF");
        }

        if (ventState == 1) {
            snprintf_s(VentSt, sizeof(VentSt), sizeof(VentSt) - 1, "ON");
        } else {
            snprintf_s(VentSt, sizeof(VentSt), sizeof(VentSt) - 1, "OFF");
        }

        osDelay(DELAY_TIME_MS);
        // osal_msleep(1000);  //每1秒采集一次
     }

    return NULL;
}


//本函数处理GPIO输出灯泡，电机的初始化，GPIO输入属性的初始化
static void gpio_init(void)
{

}

//本函数处理环境传感器的初始化、显示屏、串口的初始化
static void environment_sensor_init(void)
{
    dht11_init();
    oled_init();
    adc_init();
    led_init();
    Buzzer_Init();
    mq2_init();

    strcpy(LampSt, "OFF");
    strcpy(CondiSt, "OFF");
    strcpy(VentSt, "OFF");
    strcpy(BuzzerSt, "OFF");
    strcpy(SmokeSt, "0");
    strcpy(CO2St, "400");
}




static void *appmain_start(const char *argument)
{
    unused(argument);

    gpio_init();    //完成gpio输出相关的初始化，部分输入KEY的初始化
    environment_sensor_init();//完成采集传感器的初始化、显示屏和串口初始化

    /* 蜂鸣器启动提示：短鸣三次 */
    for (int i = 0; i < 3; i++) {
        Buzzer_On();
        osal_msleep(100);
        Buzzer_Off();
        osal_msleep(100);
    }

    wifi_connect(); //连接WIFI热点

    return NULL;
}



static void app_main(void)
{
    printf(" HUAWEI IOT BEGIN.....\r\n");

   // osDelay(DELAY_TIME_MS);   //延时100Ms
    osal_kthread_lock();
        osal_task *task1 = osal_kthread_create((osal_kthread_handler)appmain_start, 0, "appmain_start", 0x1000);
        osal_kthread_set_priority(task1, 10);
        printf("Create appmain_start succ.\r\n");

        osal_task *task2 = osal_kthread_create((osal_kthread_handler)environment_task, 0, "Environment_task", 0x1000);
        osal_kthread_set_priority(task2, 10);
        printf("Create Environment_task succ.\r\n");
  	osal_kthread_unlock();   
}

/* Run the app_main. */
app_run(app_main);