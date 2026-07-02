# 智慧房间 - 本地后端服务

## 项目概述

本后端服务是 WS63 智慧房间 IoT 系统的**本地数据处理与可视化层**。它通过华为云 IoTDA 的 AMQP 队列消费设备上报的传感器数据，存储到金仓数据库（KingbaseES），并通过 Web 界面提供实时监控和历史数据查询。

### 架构

```
WS63 设备 ──MQTT──→ 华为云 IoTDA ──AMQP 1.0──→ [本地后端服务] ──→ KingbaseES
                                                     │
                  Web 浏览器 ←── HTTP/WebSocket ────┘
```

### 数据源模式

| 模式 | 说明 | 适用场景 |
|------|------|----------|
| `amqp` | 华为云 AMQP 1.0 队列消费（当前使用） | **生产推荐方案** |
| `simulate` | 模拟生成传感器数据 | 开发测试/演示 |
| `mqtt` | 直连 MQTT 监听设备上报 | 不推荐（会与设备抢占连接导致下线） |

## 快速启动

### 前提条件

- Python 3.9+
- KingbaseES V009R001 数据库已安装并运行（端口 54321）
- 华为云 IoTDA 实例已配置 AMQP 转发规则

### 1. 安装依赖

```powershell
cd backend
pip install -r requirements.txt
```

核心依赖：
- `fastapi` + `uvicorn` — Web 框架
- `python-qpid-proton` — AMQP 1.0 客户端（华为云 IoTDA 消费）
- `psycopg2-binary` — KingbaseES 数据库驱动

### 2. 启动服务

```powershell
python -m uvicorn app:app --host 0.0.0.0 --port 8080
```

或直接运行：

```powershell
python app.py
```

### 3. 访问 Web 界面

打开浏览器访问：`http://localhost:8080`

### 4. 局域网其他设备访问

同一局域网下的手机/平板，通过 `http://<电脑IP>:8080` 访问。查看电脑 IP：

```powershell
ipconfig
```

## 配置说明

所有配置通过环境变量或编辑 `config.py` 修改。当前默认配置已对接实际环境。

### 数据库配置（KingbaseES）

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `DB_TYPE` | `kingbasees` | 数据库类型（`kingbasees` 或 `sqlite`） |
| `KINGBASE_HOST` | `127.0.0.1` | 金仓数据库地址 |
| `KINGBASE_PORT` | `54321` | 金仓数据库端口 |
| `KINGBASE_DB` | `smart_room` | 数据库名 |
| `KINGBASE_USER` | `system` | 用户名 |
| `KINGBASE_PASSWORD` | `123456` | 密码 |

切换回 SQLite 用于快速测试：

```powershell
$env:DB_TYPE="sqlite"
python -m uvicorn app:app --host 0.0.0.0 --port 8080
```

### AMQP 数据源配置

| 配置项 | 当前值 | 说明 |
|--------|--------|------|
| `DATA_SOURCE` | `amqp` | 数据源类型 |
| `AMQP_HOST` | `412aa055e1.st1.iotda-app.cn-north-4.myhuaweicloud.com` | AMQPS 接入地址 |
| `AMQP_PORT` | `5671` | AMQP TLS 端口 |
| `AMQP_QUEUE` | `roomone-data` | 消息队列名 |
| `AMQP_ACCESS_KEY` | `sySNddDM` | 预置接入凭证 Key |
| `AMQP_ACCESS_CODE` | `zchIjh859R1EeTMz63rmuXUdtoolVO6m` | 预置接入凭证 Code |
| `IOTDA_INSTANCE_ID` | `79988786-c17c-4c18-ba29-daf91e189a4d` | 标准版实例 ID |
| `IOTDA_DEVICE_ID` | `6a3a6e0d7f2e6c302f7e2ac9_roomone` | 设备 ID |

### 数据入库采样间隔

设备每 2 秒上报一次属性，但不需要每次都写数据库。后端按固定间隔采样入库，中间消息只更新内存中的最新值（Web 界面仍实时刷新）。

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `AMQP_DB_INTERVAL` | `30` | 数据库写入间隔（秒），每 30 秒入库一条 |

```powershell
# 改为 10 秒入库一次
$env:AMQP_DB_INTERVAL="10"
```

## REST API 接口

| 接口 | 方法 | 说明 |
|------|------|------|
| `/` | GET | Web 前端仪表盘 |
| `/api/v1/devices` | GET | 设备列表 |
| `/api/v1/devices/{id}` | GET | 设备详情 |
| `/api/v1/devices/{id}` | PUT | 更新设备信息 |
| `/api/v1/devices/{id}` | DELETE | 删除设备 |
| `/api/v1/latest` | GET | 最新一条传感器数据 |
| `/api/v1/history?limit=100` | GET | 历史数据查询 |
| `/api/v1/statistics` | GET | 统计数据（总记录数等） |
| `/api/v1/commands` | GET | 设备控制命令日志 |
| `/api/v1/commands` | POST | 下发设备命令 |
| `/api/v1/commands/{id}` | PUT | 更新命令状态 |
| `/api/v1/commands/{id}` | DELETE | 删除命令记录 |
| `/api/v1/health` | GET | 健康检查 |
| `/ws/realtime` | WebSocket | 实时数据推送 |

### API 示例

```powershell
# 获取最新数据
Invoke-RestMethod -Uri "http://localhost:8080/api/v1/latest"

# 获取最近 100 条历史数据
Invoke-RestMethod -Uri "http://localhost:8080/api/v1/history?limit=100"

# 获取设备列表
Invoke-RestMethod -Uri "http://localhost:8080/api/v1/devices"

# 获取统计数据
Invoke-RestMethod -Uri "http://localhost:8080/api/v1/statistics"
```

## 数据库

### 数据库表结构

| 表名 | 说明 |
|------|------|
| `device_info` | 设备信息表（设备 ID、名称、类型、位置、在线状态） |
| `sensor_data` | 传感器数据表（温度、湿度、光照、烟雾、CO2 等） |
| `device_commands` | 设备命令记录表（命令内容、参数、状态） |

### KingbaseES 初始化

首次使用金仓数据库时，执行初始化：

```powershell
# 1. 确保 KingbaseES 服务已启动（端口 54321）

# 2. 后端首次启动时会自动建表（init_database 函数）

# 3. 如需手动重建，用 KStudio 或 ksql 执行 init_kingbasees.sql
ksql -h 127.0.0.1 -p 54321 -U system -d smart_room -f init_kingbasees.sql
```

### 清空数据库

```sql
-- 保留设备信息，清空传感器数据和命令记录
DELETE FROM sensor_data;
DELETE FROM device_commands;
UPDATE device_info SET status = 'offline';
```

## 华为云 IoTDA 配置

### AMQP 对接流程

1. **预置 AMQP 接入凭证**：IoTDA 控制台 → 实例详情 → 接入凭证 → 预置，获取 `accessKey` 和 `accessCode`
2. **创建 AMQP 消息队列**：规则引擎 → 数据转发 → AMQP → 新建队列（如 `roomone-data`）
3. **创建数据转发规则**：
   - 数据来源：**设备属性**
   - 触发事件：**设备属性上报**
   - 转发目标：AMQP 推送消息队列
   - 目标队列：选择已创建的队列
4. **激活规则**：规则状态必须为"已激活"
5. **获取 AMQPS 接入地址**：实例总览 → 接入信息 → AMQPS 接入地址

### 关键注意事项

- 触发事件必须选 **"设备属性上报"**（数据来源为"设备属性"），不能选"设备消息上报"
- AMQP 使用 **1.0 协议**（非 0-9-1），认证格式为 `accessKey=xxx|timestamp=毫秒|instanceId=xxx`
- 单个 accessKey 最多支持 **32 个并发连接**

## 项目结构

```
backend/
├── app.py              # 主程序（FastAPI + WebSocket）
├── config.py           # 全局配置（数据库、AMQP、HTTP）
├── database.py         # 数据库操作层（支持 SQLite + KingbaseES 双模式）
├── data_consumer.py    # 数据消费者（AMQP 1.0 / MQTT / 模拟数据）
├── requirements.txt    # Python 依赖
├── init_kingbasees.sql # KingbaseES 建表脚本
├── .env.local          # 本地环境配置模板（不提交 Git）
├── static/
│   └── index.html      # Web 前端仪表盘
└── README.md           # 本文件
```

## 常见问题

**Q: 启动报错 "ModuleNotFoundError"？**
A: 执行 `pip install -r requirements.txt` 安装所有依赖。

**Q: AMQP 连接成功但收不到数据？**
A: 检查 IoTDA 控制台的转发规则：数据来源必须选"设备属性"，触发事件为"设备属性上报"，且规则已激活。

**Q: 金仓数据库连接失败？**
A: 确认 KingbaseES 服务已启动（`netstat -ano | findstr 54321`），且 `smart_room` 数据库已创建。

**Q: Web 页面数据一直为 "--"？**
A: 检查后端日志，确认 AMQP 是否连接成功、设备是否在线上报数据。

**Q: 如何修改 Web 端口？**
A: 设置环境变量 `HTTP_PORT` 或修改 `config.py` 中的 `HTTP_PORT`。

**Q: 数据库写入太频繁怎么办？**
A: 调大 `AMQP_DB_INTERVAL`（单位秒），如 `$env:AMQP_DB_INTERVAL="60"` 改为每分钟入库一次。
