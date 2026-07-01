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
#include "hal_timer.h"

#include "dht11.h"
static pin_t dht11_pin;
extern  DHT11_Data_TypeDef DHT11_Data;  //存放温度数据
//微秒级延时: 
void my_udelay(uint16_t us)
{
	for (volatile uint16_t i = 0; i < us; i++)
	{
		volatile uint16_t j = 40;
		while (j--)
			;
	}
}

//接收数据阶段，从DHT11中读取 8bit 的数据
static uint8_t DHT11_ReadByte(void)
{
	uint8_t i, temp = 0;
	uint16_t timeout = 0;

	for (i = 0; i < 8; i++)
	{
        //每次脉冲以50us低电平开始，等待低电平结束
		while (uapi_gpio_get_val(dht11_pin) == GPIO_LEVEL_LOW);

		//DHT11 以26~28us的高电平表示“0”，以70us高电平表示“1”, 等待35us后判断电平
		my_udelay(35); 

        //如果是高电平，说明收到的1bit的数值1
		if (uapi_gpio_get_val(dht11_pin) == GPIO_LEVEL_HIGH)
		{
            //等待高电平结束
			timeout = 0;
			while (uapi_gpio_get_val(dht11_pin) == GPIO_LEVEL_HIGH)
			{
				timeout++;
				if (timeout > 200)
				{
					osal_kthread_unlock();
					return (uint8_t)ERRCODE_FAIL;
				}
				my_udelay(1);
			}

            //高位先出
			temp |= (uint8_t)(0x01 << (7 - i)); // 把第7-i位置1，MSB先行
		}
        //temp初值为0,收到1bit的数值0时，不需要写入temp。
		
	}
	
	return temp;
}

//DHT11传感器初始化
void dht11_init(void)
{
    //指定DHT11_PIN端口
    
    dht11_pin = GPIO_04;

    //设置IO4引脚为普通IO模式
    uapi_pin_set_mode(dht11_pin, PIN_MODE_2);

    //设置IO4为输出模式
    uapi_gpio_set_dir(dht11_pin, GPIO_DIRECTION_OUTPUT);

    //设置引脚上拉
    uapi_pin_set_pull(dht11_pin, PIN_PULL_TYPE_UP);

    //设置引脚为高电平
    uapi_gpio_set_val(dht11_pin, GPIO_LEVEL_HIGH);
}

//DHT11读取温湿度信息
errcode_t dht11_read_data(DHT11_Data_TypeDef *DHT11_Data)
{
    uint8_t temp;
	uint16_t humi_temp;
	uint16_t timeout = 0;
    errcode_t result;

    // 关闭任务调度，防止读取过程中被打断
	osal_kthread_lock();

    // 启动信号,DHT11引脚至少拉低18ms
	uapi_gpio_set_dir(dht11_pin, GPIO_DIRECTION_OUTPUT);// 设置输出模式
    uapi_gpio_set_val(dht11_pin, GPIO_LEVEL_LOW); // 引脚电平拉低 
    osal_mdelay(18);// 延时18ms
    uapi_gpio_set_val(dht11_pin, GPIO_LEVEL_HIGH); // 引脚电平拉高

    // 响应信号, DHT11先向Hi3863发送80us低电平,再发送80us高电平,最后发送40bit的数据
    uapi_gpio_set_dir(dht11_pin, GPIO_DIRECTION_INPUT);   //设置输入模式
	uapi_pin_set_pull(dht11_pin, PIN_PULL_TYPE_STRONG_UP);//设置强上拉

    //等待DHT11将电平拉低，最多等待200us
    timeout = 0;
    while (uapi_gpio_get_val(dht11_pin) == GPIO_LEVEL_HIGH)
    {
        timeout++;
        if(timeout > 200)
        {
            printf("DHT11 dht11_read_data failed 1");
            osal_kthread_unlock();//启动任务调度
            return ERRCODE_FAIL;
        }
        my_udelay(1);
    }

    //等待DHT11将电平拉高,最多等待200us
    timeout = 0;
    while (uapi_gpio_get_val(dht11_pin) == GPIO_LEVEL_LOW)
    {
        timeout++;
        if(timeout > 200)
        {
            printf("DHT11 dht11_read_data failed 2");
            osal_kthread_unlock();//启动任务调度
            return ERRCODE_FAIL;
        }
        my_udelay(1);
    }

    //数据传输阶段的起始电平是低电平,等待DHT11将电平拉低
    timeout = 0;
    while (uapi_gpio_get_val(dht11_pin) == GPIO_LEVEL_HIGH)
    {
        timeout++;
        if(timeout > 200)
        {
            printf("DHT11 dht11_read_data failed 3");
            osal_kthread_unlock();//启动任务调度
            return ERRCODE_FAIL;
        }
        my_udelay(1);
    }

    /*开始接收数据*/
	DHT11_Data->humi_high8bit = DHT11_ReadByte();
	DHT11_Data->humi_low8bit = DHT11_ReadByte();
	DHT11_Data->temp_high8bit = DHT11_ReadByte();
	DHT11_Data->temp_low8bit = DHT11_ReadByte();
	DHT11_Data->check_sum = DHT11_ReadByte();

    //读取结束，引脚改为输出模式
	uapi_gpio_set_dir(dht11_pin, GPIO_DIRECTION_OUTPUT);
    //引脚电平拉高,停止
    uapi_gpio_set_val(dht11_pin, GPIO_LEVEL_HIGH);

    //计算湿度数据
    humi_temp = DHT11_Data->humi_high8bit * 100 + DHT11_Data->humi_low8bit;
	DHT11_Data->humidity = (float)humi_temp / 100;

    //计算温度数据
    humi_temp = DHT11_Data->temp_high8bit * 100 + DHT11_Data->temp_low8bit;
	DHT11_Data->temperature = (float)humi_temp / 100;

    //校验结果
    temp = DHT11_Data->humi_high8bit + DHT11_Data->humi_low8bit + DHT11_Data->temp_high8bit + DHT11_Data->temp_low8bit;
    if( temp == DHT11_Data->check_sum)
    {
        result = ERRCODE_SUCC;//成功
    }
    else
    {
        result = ERRCODE_FAIL;//失败
    }

    osal_kthread_unlock();//启动任务调度
    return result;
}
