# MQ-2 Smoke Sensor 烟雾传感器驱动 (WS63 / Hi3863)

本驱动用于在基于 WS63 (Hi3863) 星闪芯片的 OpenHarmony/LiteOS 系统上读取**MQ-2 烟雾传感器**的模拟值。

## 文件组成
- `mq2.h`: MQ-2传感器驱动头文件，定义接口、ADC通道和引脚宏。
- `mq2.c`: MQ-2传感器驱动源码。

## 依赖
本驱动依赖 OpenHarmony 的标准 ADC 和 GPIO API：
- `iot_adc.h`
- `iot_gpio.h`

## 使用说明

### 1. 硬件连接
请将 MQ-2 传感器的 **A0 (模拟输出)** 引脚连接到 WS63 开发板支持 ADC 采集的引脚上。
默认配置使用 **ADC通道 2** (假设对应 `GPIO_4`)。您可以在 `mq2.h` 中修改 `MQ2_ADC_CHANNEL` 和 `MQ2_GPIO_PIN` 的值以匹配实际硬件连线。

- **VCC** 接 开发板 5V (MQ-2 通常需要 5V 供电以加热内部传感元件)
- **GND** 接 开发板 GND
- **A0 (模拟信号)** 接 开发板 ADC 输入引脚 (例如: `ADC通道 2`)

### 2. 初始化与控制
在您的应用代码中包含 `mq2.h`，然后按需调用接口获取烟雾浓度数据。

```c
#include "mq2.h"
#include "buzzer.h"
#include "cmsis_os2.h" // 如果使用osDelay
#include <stdio.h>

void AppTask(void)
{
    // 1. 初始化传感器
    MQ2_Init();
    
    while(1) {
        // 2. 获取烟雾浓度百分比
        uint8_t smog = MQ2_GetPercentage();
        
        printf("Smog: %d %%\r\n", smog);
        
        // 3. 超过一定浓度触发报警 (根据原版逻辑，值大于4即可触发报警)
        if (smog > 4) {
            printf("Smog Warn: Warn!\r\n");
            // Buzzer_On();
        } else {
            // Buzzer_Off();
        }
        
        osDelay(100);
    }
}
```
