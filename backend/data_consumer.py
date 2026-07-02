"""
==========================================================================
                    智慧房间 - 数据消费者模块
                    版权所有：沈阳市网联通信规划设计有限公司
==========================================================================
支持三种数据源：
1. simulate - 模拟数据（默认，无需网络连接即可演示）
2. amqp     - 华为云 AMQP 1.0 队列消费（推荐的生产方案）
3. mqtt     - 直连 MQTT 监听设备上报（不推荐：会与设备抢占连接）
"""

import json
import logging
import random
import threading
import time
from datetime import datetime, timezone, timedelta

import config
import database

logger = logging.getLogger(__name__)

# 北京时区
BJT = timezone(timedelta(hours=8))

# 存储最新一条数据的全局变量（供 WebSocket 推送使用）
latest_data = {}
_data_lock = threading.Lock()


def get_latest_data():
    """获取最新数据快照"""
    with _data_lock:
        return dict(latest_data) if latest_data else {}


def _update_latest(data: dict):
    """更新最新数据快照"""
    with _data_lock:
        latest_data.clear()
        latest_data.update(data)


# ---------- 数据解析 ----------

def parse_iotda_properties(payload: str) -> dict:
    """
    解析华为云 IoTDA 设备属性上报的 JSON 数据
    支持两种格式：
    1. 标准格式: {"services":[{"service_id":"smartRoom","properties":{...}}]}
    2. 扁平格式: {"Temp":"28.5","Humi":"60.2",...}
    """
    try:
        obj = json.loads(payload)
    except json.JSONDecodeError:
        logger.warning(f"无法解析JSON: {payload[:200]}")
        return {}

    # 标准 IoTDA 格式
    if "services" in obj and isinstance(obj["services"], list):
        for svc in obj["services"]:
            if "properties" in svc:
                return svc["properties"]
        return {}

    # 扁平格式
    return obj


def parse_mqtt_payload(topic: str, payload: str) -> dict:
    """解析 MQTT 收到的原始消息"""
    return parse_iotda_properties(payload)


def parse_amqp_message(message: dict) -> dict:
    """解析 AMQP 收到的消息（华为云 IoTDA 格式）"""
    # 华为云 AMQP 消息格式：
    # {"resource": "device.property", "event": "report", ...,
    #  "notify_data": {"body": {"services": [...]}}}
    try:
        notify = message.get("notify_data", {})
        body_str = notify.get("body", "{}")
        if isinstance(body_str, str):
            body = json.loads(body_str)
        else:
            body = body_str
        return parse_iotda_properties(json.dumps(body))
    except Exception as e:
        logger.error(f"解析AMQP消息失败: {e}")
        return {}


# ---------- 模拟数据生成器 ----------

class SimulateDataSource:
    """模拟数据源 - 生成逼真的传感器数据用于演示"""

    def __init__(self, interval: int = 5):
        self.interval = interval
        self._running = False
        self._thread = None

        # 初始值
        self.temp = 26.5
        self.humi = 58.0
        self.lumi = 300
        self.smoke = 0
        self.co2 = 400
        self.lamp_st = "ON" if self.lumi < 50 else "OFF"
        self.cond_st = "OFF"
        self.vent_st = "OFF"
        self.buzzer_st = "OFF"

    def start(self):
        """启动模拟数据生成线程"""
        if self._running:
            return
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        logger.info(f"模拟数据源已启动，每 {self.interval} 秒生成一条数据")

    def stop(self):
        """停止模拟数据生成"""
        self._running = False

    def _simulate_once(self) -> dict:
        """生成一条模拟数据"""
        # 温度：26~30 度之间缓慢变化
        self.temp += random.uniform(-0.3, 0.3)
        self.temp = max(20.0, min(35.0, self.temp))

        # 湿度：50~70 之间缓慢变化
        self.humi += random.uniform(-1.0, 1.0)
        self.humi = max(40.0, min(80.0, self.humi))

        # 光照：白天 200~800，夜晚 0~50
        hour = datetime.now(BJT).hour
        if 6 <= hour < 18:
            self.lumi += random.randint(-20, 20)
            self.lumi = max(100, min(900, self.lumi))
        else:
            self.lumi += random.randint(-5, 5)
            self.lumi = max(0, min(60, self.lumi))

        # 烟雾：偶尔波动
        self.smoke = max(0, self.smoke + random.randint(-1, 2))
        self.smoke = min(100, self.smoke)

        # CO2：400~800
        self.co2 += random.randint(-20, 20)
        self.co2 = max(350, min(1000, self.co2))

        # 灯控：根据光照自动
        if self.lumi < 50:
            self.lamp_st = "ON"
        elif self.lumi > 200:
            self.lamp_st = "OFF"
        # 空调：保持状态

        return {
            "device_id": "roomone",
            "Temp": f"{self.temp:.1f}",
            "Humi": f"{self.humi:.1f}",
            "Lumi": str(int(self.lumi)),
            "LampST": self.lamp_st,
            "CondST": self.cond_st,
            "VentST": self.vent_st,
            "BuzzerST": self.buzzer_st,
            "Smoke": str(int(self.smoke)),
            "CO2": str(int(self.co2)),
        }

    def _run(self):
        """模拟数据生成主循环"""
        while self._running:
            try:
                data = self._simulate_once()
                # 更新设备在线状态
                database.update_device(data.get("device_id", "roomone"), status="online")
                _update_latest(data)
                database.insert_sensor_data(data)
                logger.debug(f"模拟数据已存储: Temp={data['Temp']}, Humi={data['Humi']}")
            except Exception as e:
                logger.error(f"模拟数据存储失败: {e}")
            time.sleep(self.interval)


# ---------- MQTT 数据消费者 ----------

class MqttDataSource:
    """MQTT 数据消费者 - 直连华为云 IoTDA MQTT Broker"""

    def __init__(self):
        self._running = False
        self._thread = None
        self._client = None
        self._connected = False

    def start(self):
        """启动 MQTT 消费线程"""
        if self._running:
            return
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        logger.info("MQTT 数据消费者已启动")

    def stop(self):
        self._running = False
        if self._client:
            try:
                self._client.disconnect()
            except Exception:
                pass

    def _on_connect(self, client, userdata, flags, rc):
        """MQTT 连接回调"""
        if rc == 0:
            self._connected = True
            logger.info("MQTT 连接成功 (返回码=0)")
            # 连接成功后再订阅
            result = client.subscribe(config.MQTT_TOPIC, qos=1)
            logger.info(f"MQTT 订阅请求已发送: {config.MQTT_TOPIC}, result={result}")
        elif rc == 1:
            logger.error("MQTT 连接失败: 协议版本错误")
        elif rc == 2:
            logger.error("MQTT 连接失败: 客户端ID被拒")
        elif rc == 3:
            logger.error("MQTT 连接失败: 服务器不可用")
        elif rc == 4:
            logger.error("MQTT 连接失败: 用户名或密码错误")
        elif rc == 5:
            logger.error("MQTT 连接失败: 未授权")
        else:
            logger.error(f"MQTT 连接失败: 未知返回码 {rc}")
        # 连接失败时尝试重连
        if rc != 0:
            logger.info("将在 5 秒后重试连接...")
            time.sleep(5)
            try:
                client.reconnect()
            except Exception as e:
                logger.error(f"MQTT 重连失败: {e}")

    def _on_disconnect(self, client, userdata, rc):
        """MQTT 断开回调"""
        self._connected = False
        if rc != 0:
            logger.warning(f"MQTT 意外断开 (返回码={rc})，尝试重连...")
            try:
                client.reconnect()
            except Exception as e:
                logger.error(f"MQTT 重连失败: {e}")
        else:
            logger.info("MQTT 正常断开")

    def _on_subscribe(self, client, userdata, mid, granted_qos):
        """MQTT 订阅回调"""
        logger.info(f"MQTT 订阅成功 (mid={mid}, qos={granted_qos})")

    def _on_message(self, client, userdata, msg):
        """MQTT 消息回调"""
        try:
            payload_str = msg.payload.decode("utf-8")
            logger.info(f"MQTT 原始消息: topic={msg.topic}, payload={payload_str[:300]}")
            data = parse_mqtt_payload(msg.topic, payload_str)
            if data:
                # 从订阅主题中提取设备ID（格式: .../devices/{device_id}/sys/...）
                device_id = "roomone"
                if "/devices/" in msg.topic:
                    parts = msg.topic.split("/devices/")
                    if len(parts) > 1:
                        device_id = parts[1].split("/")[0]
                data["device_id"] = device_id
                # 更新设备在线状态 + 写入传感器数据
                database.update_device(device_id, status="online")
                _update_latest(data)
                database.insert_sensor_data(data)
                logger.info(f"MQTT 收到数据: device={device_id}, Temp={data.get('Temp')}, Humi={data.get('Humi')}")
            else:
                logger.warning(f"MQTT 数据解析为空: payload={payload_str[:200]}")
        except Exception as e:
            logger.error(f"MQTT 消息处理失败: {e}")

    def _run(self):
        """MQTT 连接与消费（含自动重连）"""
        retry_count = 0
        while self._running:
            try:
                import paho.mqtt.client as mqtt

                # 生成唯一客户端ID（避免与设备冲突）
                import uuid
                client_id = config.MQTT_CLIENT_ID + "_" + uuid.uuid4().hex[:8]

                self._client = mqtt.Client(client_id=client_id, clean_session=True)
                self._client.username_pw_set(config.MQTT_USERNAME, config.MQTT_PASSWORD)
                self._client.on_connect = self._on_connect
                self._client.on_disconnect = self._on_disconnect
                self._client.on_subscribe = self._on_subscribe
                self._client.on_message = self._on_message

                # 设置遗嘱消息（客户端意外断开时通知）
                self._client.will_set(
                    f"$oc/devices/{config.MQTT_USERNAME}/sys/messages/events",
                    payload='{"backend_status":"disconnected"}',
                    qos=1, retain=False
                )

                logger.info(f"正在连接 MQTT Broker: {config.MQTT_BROKER}:{config.MQTT_PORT}")
                self._client.connect(config.MQTT_BROKER, config.MQTT_PORT, keepalive=60)

                # 进入事件循环
                while self._running:
                    rc = self._client.loop(timeout=1.0)
                    if rc != 0:
                        logger.warning(f"MQTT 循环异常退出 (rc={rc})，准备重连...")
                        break

            except ImportError:
                logger.error("paho-mqtt 未安装，请执行: pip install paho-mqtt")
                break
            except Exception as e:
                retry_count += 1
                logger.error(f"MQTT 连接/消费失败 (第{retry_count}次): {e}")

            # 自动重连
            if self._running:
                wait = min(retry_count * 5, 60)  # 递增等待，最多60秒
                logger.info(f"将在 {wait} 秒后重试...")
                for i in range(wait):
                    if not self._running:
                        break
                    time.sleep(1)


# ---------- AMQP 1.0 数据消费者（华为云 IoTDA 推荐方案） ----------

class AmqpDataSource:
    """
    AMQP 1.0 数据消费者 - 连接华为云 IoTDA AMQP 队列

    数据流：设备上报 → IoTDA → 规则引擎转发 → AMQP 队列 → 本地后端

    华为云 IoTDA 使用 AMQP 1.0 协议（非 0-9-1），需要 qpid-proton 库。
    认证格式：username = accessKey=xxx|timestamp=毫秒|instanceId=xxx
              password = accessCode
    """

    def __init__(self):
        self._running = False
        self._thread = None
        self._last_db_time = 0  # 上次入库时间戳

    def start(self):
        if self._running:
            return
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        logger.info("AMQP 1.0 数据消费者已启动")

    def stop(self):
        self._running = False

    def _build_username(self) -> str:
        """构造华为云 AMQP 认证用户名
        格式: accessKey={accessKey}|timestamp={毫秒时间戳}|instanceId={实例ID}
        """
        ts = int(time.time() * 1000)
        return (
            f"accessKey={config.AMQP_ACCESS_KEY}"
            f"|timestamp={ts}"
            f"|instanceId={config.IOTDA_INSTANCE_ID}"
        )

    def _process_message(self, body: bytes) -> bool:
        """处理一条 AMQP 消息，返回 True 表示处理成功"""
        try:
            body_str = body.decode("utf-8") if isinstance(body, bytes) else str(body)
            logger.debug(f"AMQP 原始消息: {body_str[:300]}")

            msg = json.loads(body_str)
            data = parse_amqp_message(msg)

            if not data:
                logger.warning(f"AMQP 数据解析为空: {body_str[:200]}")
                return True  # 消息格式不对，仍然 ack 掉避免堆积

            # 从消息中提取设备ID
            device_id = msg.get("device_id", config.IOTDA_DEVICE_ID)
            if not device_id:
                device_id = config.IOTDA_DEVICE_ID
            data["device_id"] = device_id.split("_")[-1] if "_" in device_id else device_id

            # 每条消息都更新内存中的最新值（Web 界面实时看到）
            _update_latest(data)

            # 按采样间隔写入数据库（减轻数据库压力）
            now = time.time()
            if now - self._last_db_time >= config.AMQP_DB_INTERVAL:
                database.update_device(data["device_id"], status="online")
                database.insert_sensor_data(data)
                self._last_db_time = now
                logger.info(
                    f"AMQP 入库: device={data['device_id']}, "
                    f"Temp={data.get('Temp')}, Humi={data.get('Humi')}"
                )
            else:
                logger.debug(
                    f"AMQP 跳过入库: Temp={data.get('Temp')}, Humi={data.get('Humi')}"
                )
            return True
        except Exception as e:
            logger.error(f"AMQP 消息处理失败: {e}")
            return False

    def _run(self):
        """AMQP 1.0 消费主循环（含自动重连）"""
        retry_count = 0

        while self._running:
            try:
                from proton import Message
                from proton.reactor import Container
                from proton.handlers import MessagingHandler
            except ImportError:
                logger.error(
                    "python-qpid-proton 未安装，请执行: pip install qpid-proton"
                )
                break

            # 配置检查
            if not config.AMQP_HOST or not config.AMQP_ACCESS_KEY:
                logger.error(
                    "AMQP 配置不完整，请检查 AMQP_HOST / AMQP_ACCESS_KEY / "
                    "AMQP_ACCESS_CODE / AMQP_QUEUE / IOTDA_INSTANCE_ID"
                )
                break

            # 构造连接 URL: amqps://host:port
            url = f"amqps://{config.AMQP_HOST}:{config.AMQP_PORT}"
            username = self._build_username()
            password = config.AMQP_ACCESS_CODE
            queue = config.AMQP_QUEUE

            logger.info(f"AMQP 正在连接: {url}, 队列: {queue}")

            # 定义消息处理器
            class AmqpConsumer(MessagingHandler):
                def __init__(self_outer):
                    super().__init__()
                    self_outer.parent = self
                    self_outer.connected = False

                def on_start(self_outer, event):
                    conn = event.container.connect(
                        url=url,
                        user=username,
                        password=password,
                        sasl_enabled=True,
                        allowed_mechs="PLAIN",
                    )
                    event.container.create_receiver(conn, source=queue)

                def on_connection_opened(self_outer, event):
                    if not self_outer.connected:
                        self_outer.connected = True
                        retry_count = 0
                        logger.info(f"AMQP 连接成功: {url}")

                def on_message(self_outer, event):
                    body = event.message.body
                    self_outer.parent._process_message(body)

                def on_connection_closed(self_outer, event):
                    logger.warning("AMQP 连接已关闭")

                def on_transport_error(self_outer, event):
                    cond = event.condition
                    desc = cond.description if cond else "未知错误"
                    logger.error(f"AMQP 传输错误: {desc}")

                def on_disconnected(self_outer, event):
                    self_outer.connected = False
                    logger.warning("AMQP 连接断开，准备重连...")

            try:
                container = Container(AmqpConsumer())
                container.run()
            except Exception as e:
                logger.error(f"AMQP 连接/消费异常: {e}")

            # 自动重连
            if self._running:
                retry_count += 1
                wait = min(retry_count * 5, 60)
                logger.info(f"将在 {wait} 秒后重试 AMQP 连接 (第{retry_count}次)...")
                for _ in range(wait):
                    if not self._running:
                        break
                    time.sleep(1)


# ---------- 工厂函数 ----------

def create_data_source():
    """根据配置创建对应的数据源实例"""
    source_type = config.DATA_SOURCE.lower()

    if source_type == "amqp":
        return AmqpDataSource()
    elif source_type == "mqtt":
        return MqttDataSource()
    else:
        return SimulateDataSource(interval=config.SIMULATE_INTERVAL)