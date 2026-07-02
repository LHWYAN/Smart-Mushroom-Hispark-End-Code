"""
==========================================================================
                    智慧房间 - 本地后端服务配置
                    版权所有：沈阳市网联通信规划设计有限公司
==========================================================================
本配置文件包含数据库、MQTT/AMQP、HTTP服务等所有可调参数。
部署前请根据实际环境修改以下配置项。
"""

import os

# ==================== 数据库配置 ====================
# 支持两种数据库：sqlite（默认，无需额外安装） 或 kingbasees（人大金仓）
DB_TYPE = os.getenv("DB_TYPE", "kingbasees")  # "sqlite" 或 "kingbasees"

# --- SQLite 配置 ---
# 数据库文件存储路径，默认保存在 backend 目录下
SQLITE_DB_PATH = os.getenv(
    "SQLITE_DB_PATH",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "sensor_data.db"),
)

# --- KingbaseES (人大金仓) 配置 ---
# 当 DB_TYPE = "kingbasees" 时生效
KINGBASE_HOST = os.getenv("KINGBASE_HOST", "127.0.0.1")
KINGBASE_PORT = int(os.getenv("KINGBASE_PORT", "54321"))
KINGBASE_DB = os.getenv("KINGBASE_DB", "smart_room")
KINGBASE_USER = os.getenv("KINGBASE_USER", "system")
KINGBASE_PASSWORD = os.getenv("KINGBASE_PASSWORD", "123456")

# AMQP 数据入库采样间隔（秒）
# 设备可能每 2 秒上报一次，但不需要每次都写数据库
# 设为 10 表示每 10 秒最多入库一次，中间的消息只更新内存中的最新值
AMQP_DB_INTERVAL = int(os.getenv("AMQP_DB_INTERVAL", "30"))


# ==================== 数据源配置 ====================
# 数据来源：simulate（模拟数据，用于演示） / amqp（华为云AMQP，推荐） / mqtt（直连MQTT，会踢设备下线，不推荐）
DATA_SOURCE = os.getenv("DATA_SOURCE", "amqp")

# --- MQTT 直连配置（不推荐：会与设备抢占同一MQTT连接，导致设备被踢下线） ---
MQTT_BROKER = os.getenv("MQTT_BROKER", "412aa055e1.st1.iotda-device.cn-north-4.myhuaweicloud.com")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "$oc/devices/6a3a6e0d7f2e6c302f7e2ac9_roomone/sys/properties/report")
MQTT_CLIENT_ID = os.getenv("MQTT_CLIENT_ID", "backend_consumer_001")
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "6a3a6e0d7f2e6c302f7e2ac9_roomone")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "c4083010cded920e7cb5c928f2ffcf0bcb214b93c54423bb80b891ef81247e2a")

# --- 华为云 AMQP 配置（推荐方案） ---
# 如果 DATA_SOURCE = "amqp"，通过 AMQP 1.0 协议从华为云 IoTDA 消费设备数据
# 需要先在 IoTDA 控制台完成：预置接入凭证 → 创建队列 → 创建转发规则并激活
AMQP_HOST = os.getenv("AMQP_HOST", "412aa055e1.st1.iotda-app.cn-north-4.myhuaweicloud.com")
AMQP_PORT = int(os.getenv("AMQP_PORT", "5671"))    # 固定 5671（TLS 加密）
AMQP_QUEUE = os.getenv("AMQP_QUEUE", "roomone-data")
AMQP_ACCESS_KEY = os.getenv("AMQP_ACCESS_KEY", "sySNddDM")
AMQP_ACCESS_CODE = os.getenv("AMQP_ACCESS_CODE", "zchIjh859R1EeTMz63rmuXUdtoolVO6m")

# --- IoTDA 实例信息（AMQP 认证需要） ---
# 标准版实例ID（实例总览页可查看）
IOTDA_INSTANCE_ID = os.getenv("IOTDA_INSTANCE_ID", "79988786-c17c-4c18-ba29-daf91e189a4d")
# 设备ID（用于设备状态更新等辅助操作）
IOTDA_DEVICE_ID = os.getenv("IOTDA_DEVICE_ID", "6a3a6e0d7f2e6c302f7e2ac9_roomone")


# ==================== HTTP 服务配置 ====================
HTTP_HOST = os.getenv("HTTP_HOST", "0.0.0.0")  # 监听所有网卡，局域网设备可访问
HTTP_PORT = int(os.getenv("HTTP_PORT", "8080"))


# ==================== 模拟数据配置 ====================
# 模拟数据生成间隔（秒）
SIMULATE_INTERVAL = int(os.getenv("SIMULATE_INTERVAL", "30"))