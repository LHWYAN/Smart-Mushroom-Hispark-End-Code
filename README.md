# Smart Mushroom Hispark End Code

基于 **Hi3863/WS63** 芯片的 **华为云 IoT 智慧房间** 端侧演示系统。设备端采集环境温湿度、光照强度等数据，通过 MQTT 协议上报至华为云 IoT 平台，并响应平台下发的远程控制命令。

---

## 硬件需求

| 组件 | 型号 / 规格 | 数量 |
|------|-------------|------|
| 主控板 | WS63 / Hi3863 开发板 | 1 |
| 温湿度传感器 | DHT11 | 1 |
| OLED 显示屏 | 0.96寸 SSD1306（128x64，I2C） | 1 |
| 光敏电阻模块 | 模拟输出（接 ADC） | 1 |
| LED | 5mm 发光二极管 | 2 |
| 有源蜂鸣器 | 低电平触发 | 1 |
| 超声波测距模块 | HC-SR04（预留，与蜂鸣器共用 GPIO_09） | 1（可选） |
| 排风扇 / 空调 | 继电器模块模拟（预留） | 各 1（可选） |

### 引脚分配

| 外设 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| LED1 | GPIO_02 | 输出 (PIN_MODE_0) | 高电平点亮 |
| LED2 | GPIO_03 | 输出 (PIN_MODE_0) | 高电平点亮 |
| DHT11 DATA | GPIO_04 | 双向 (PIN_MODE_2) | 单总线通讯 |
| 光敏 ADC | ADC_CH5 (GPIO_05) | 模拟输入 | ADC 通道 5 |
| HC-SR04 TRIG | GPIO_06 | 输出 (PIN_MODE_0) | 超声波触发 |
| 蜂鸣器 | GPIO_09 | 输出 (PIN_MODE_0) | 低电平发声 |
| HC-SR04 ECHO | GPIO_09 | 输入 (PIN_MODE_0) | **与蜂鸣器冲突** |
| OLED SCL | GPIO_15 | I2C (PIN_MODE_2) | I2C 总线 1 |
| OLED SDA | GPIO_16 | I2C (PIN_MODE_2) | I2C 总线 1 |

> 蜂鸣器与 HC-SR04 ECHO 共用 GPIO_09，同一时间只能使用其中一个外设。如需同时使用，请修改 `buzzer/buzzer.h` 或 `hcsr04/hcsr04.h` 中的引脚定义。

---

## 软件依赖

- [Hi3863/WS63 SDK](https://device.harmonyos.com/)（含 RISC-V 工具链 `cc_riscv32_musl`）
- `python` 3.x（用于构建脚本 `build.py`）
- `make` / `ninja`（构建系统）

---

## 快速开始

### 1. 配置 SDK 环境

确保 SDK 根目录的编译工具链正确配置，Python 环境可用。

### 2. 选择目标

在 SDK 根目录执行：

```bash
# 如果 SDK 提供了 menuconfig：
cd <sdk_root>/src
python build.py menuconfig
# 在菜单中启用 ENABLE_HUAWEIIOT
```

或直接在 `Kconfig` 中确认依赖路径正确。

### 3. 编译

```bash
python build.py ws63-liteos-app
```

编译产物位于 `output/ws63/acore/ws63-liteos-app/`。

### 4. 烧录

使用 Hiburn 或 HiFlash 工具将生成的 `*.bin` 文件烧录至 WS63 开发板。烧录地址参考 SDK 文档中的分区表。

---

## 项目结构

```
21_huaweiiot/
├── app_main.c              # 主程序入口，任务创建与传感器采集循环
├── app_main.h              # MQTT / WiFi 配置（设备ID、密码等）
├── CMakeLists.txt          # 构建配置
├── Kconfig                 # 菜单配置
├── adc/
│   ├── ldr.c / ldr.h       # 光敏电阻 ADC 采集
├── buzzer/
│   ├── buzzer.c / buzzer.h # 蜂鸣器控制
├── dht11/
│   ├── dht11.c / dht11.h   # DHT11 温湿度传感器驱动
├── hcsr04/
│   ├── hcsr04.c / hcsr04.h # HC-SR04 超声波测距（预留）
├── led/
│   ├── led.c / led.h       # LED 照明控制
├── mqtt/
│   ├── mqtt.c / mqtt.h     # MQTT 华为云 IoT 通信
├── oled/
│   ├── bsp_oled.c/h        # OLED 底层 I2C 驱动
│   ├── oled.c/h            # SSD1306 初始化与绘图
│   ├── oled_fonts.c/h      # 字库（6x8 ~ 16x26）
└── wifi/
    ├── wifi_connect.c/h    # WiFi STA 连接与 MQTT 启动
```

---

## 华为云 IoT 配置

### 连接参数

所有参数定义在 `app_main.h` 中，部署前请替换为你的设备信息：

| 参数 | 示例值 |
|------|--------|
| Broker | `412aa055e1.st1.iotda-device.cn-north-4.myhuaweicloud.com` |
| 端口 | 1883 |
| 设备 ID | `6a3a6e0d7f2e6c302f7e2ac9_roomone` |
| Client ID | `{deviceId}_0_0_2026062312` |
| 密码 | 从华为云平台生成 |

### MQTT 主题

| 方向 | 主题 |
|------|------|
| 订阅命令 | `$oc/devices/{deviceId}/sys/commands/#` |
| 上报属性 | `$oc/devices/{deviceId}/sys/properties/report` |
| 命令响应 | `$oc/devices/{deviceId}/sys/commands/response/request_id=%s` |

### 上报属性

```json
{
  "services": [{
    "service_id": "smartRoom",
    "properties": {
      "Temp":    "28.5",
      "Humi":    "60.2",
      "Lumi":    "75",
      "LampST":  "ON",
      "CondST":  "OFF",
      "VentST":  "OFF",
      "BuzzerST":"OFF",
      "Smoke":   "0",
      "CO2":     "400"
    }
  }]
}
```

### 平台下发命令

| 命令 | 参数字段 | 值 | 动作 |
|------|----------|----|------|
| `SetLamp` | `LampStatus` | `ON` / `OFF` | 开关 LED 灯 |
| `SetCond` | `CondStatus` | `ON` / `OFF` | 设置空调状态 |
| `SetVent` | `VentStatus` | `ON` / `OFF` | 设置排风扇状态 |
| `SetBuzzer` | `BuzzerStatus` | `ON` / `OFF` | 开关蜂鸣器 |

---

## 系统架构

```
app_main()
├── appmain_start()            # 启动任务
│   ├── gpio_init()            # [预留]
│   ├── environment_sensor_init()
│   │   ├── dht11_init()
│   │   ├── oled_init()
│   │   ├── adc_init()
│   │   ├── led_init()
│   │   └── Buzzer_Init()
│   ├── 蜂鸣器短鸣 3 次提示
│   └── wifi_connect()
│       └── mqtt_app_start()   # WiFi 连接成功后启动
│           ├── MQTT 连接华为云
│           ├── 订阅命令主题
│           └── 循环 100ms：处理命令 + 属性上报
│
└── environment_task()          # 传感器任务 100ms
    ├── DHT11 温湿度 → OLED
    ├── ADC 光敏值 → 自动控灯 → OLED
    └── 更新设备状态标志
```

系统上电后自动运行：初始化所有外设 → 蜂鸣器提示音 → 连接 WiFi → 连接华为云 IoT → 周期性采集并上报数据，同时监听云平台下发的控制命令。

---

## 注意事项

1. **`GPIO_09` 引脚冲突**：蜂鸣器与 HC-SR04 ECHO 共用 GPIO_09。如需同时使用超声波，将蜂鸣器改为其他空闲 GPIO。
2. **WiFi 凭据硬编码**：SSID 和密码当前硬编码在 `app_main.h` 中，请根据实际环境修改。
3. **ADC 通道**：光敏电阻默认使用 ADC 通道 5，可通过 `Kconfig` 中的 `LDR_ADC_CHANNEL` 调整。
4. **MQTT 长连接**：Keep Alive 设为 120 秒，设备需保持网络稳定；连接断开后不会自动重连（当前版本）。
5. **非加密传输**：MQTT 使用 1883 端口（非 TLS），生产环境建议启用 8883 加密端口。

---

## License

本项目仅供学习使用。