"""
==========================================================================
                    智慧房间 - 本地后端服务主程序
                    版权所有：沈阳市网联通信规划设计有限公司
==========================================================================
FastAPI 后端服务，提供：
1. 数据采集层（模拟/MQTT/AMQP）-> 数据库存储
2. RESTful API 接口供 Web 前端查询
3. WebSocket 实时推送最新数据
4. 静态 Web 前端界面
5. 三表完整 CRUD 操作接口

启动方式：
    python app.py
==========================================================================
"""

import json
import requests
import logging
import os
import sys
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Optional

import uvicorn
from fastapi import FastAPI, Query, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

# 将 backend 目录加入 Python 路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import config
import database
from data_consumer import create_data_source, get_latest_data

# ---------- 日志配置 ----------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger("smart_room_backend")

# ---------- 全局变量 ----------
data_source = None
websocket_clients = set()


# ---------- 应用生命周期 ----------

@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用启动/关闭时的生命周期管理"""
    global data_source

    logger.info("=" * 50)
    logger.info("智慧房间 - 本地后端服务启动")
    logger.info(f"  数据库类型: {config.DB_TYPE}")
    logger.info(f"  数据源类型: {config.DATA_SOURCE}")
    logger.info(f"  HTTP 地址: http://{config.HTTP_HOST}:{config.HTTP_PORT}")
    logger.info("=" * 50)

    database.init_database()
    logger.info("数据库初始化完成")

    data_source = create_data_source()
    data_source.start()

    yield

    if data_source:
        data_source.stop()
    logger.info("智慧房间 - 本地后端服务已停止")


# ---------- FastAPI 应用初始化 ----------

app = FastAPI(
    title="智慧房间 - 本地后端服务",
    description="WS63 智慧房间 IoT 系统，3表架构 + 完整CRUD",
    version="2.0.0",
    lifespan=lifespan,
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

static_dir = Path(__file__).parent / "static"
static_dir.mkdir(exist_ok=True)
app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")


# ===================================================================
#  首页
# ===================================================================

@app.get("/", response_class=HTMLResponse)
async def index():
    html_path = static_dir / "index.html"
    if html_path.exists():
        return HTMLResponse(content=html_path.read_text(encoding="utf-8"))
    return HTMLResponse(content="<h1>智慧房间</h1><p>Web 前端页面未找到</p>")


# ===================================================================
#  表1: device_info CRUD API
# ===================================================================

# ----- CREATE: 注册设备 -----
@app.post("/api/v1/devices")
async def api_create_device(
    device_id: str = Query(..., description="设备唯一标识"),
    device_name: str = Query("", description="设备名称"),
    device_type: str = Query("sensor", description="设备类型"),
    location: str = Query("", description="安装位置"),
    remarks: str = Query("", description="备注"),
):
    """CREATE: 注册新设备"""
    try:
        result = database.create_device(device_id, device_name, device_type, location, remarks)
        return {"code": 0, "data": result, "message": "设备注册成功"}
    except Exception as e:
        return {"code": -1, "message": f"设备注册失败: {str(e)}"}


# ----- READ: 查询所有设备 -----
@app.get("/api/v1/devices")
async def api_get_all_devices():
    """READ: 查询所有设备"""
    devices = database.get_all_devices()
    return {"code": 0, "data": devices, "total": len(devices), "message": "success"}


# ----- READ: 查询单个设备 -----
@app.get("/api/v1/devices/{device_id}")
async def api_get_device(device_id: str):
    """READ: 查询单个设备"""
    device = database.get_device_by_id(device_id)
    if device:
        return {"code": 0, "data": device, "message": "success"}
    return {"code": -1, "message": "设备不存在"}


# ----- UPDATE: 更新设备 -----
@app.put("/api/v1/devices/{device_id}")
async def api_update_device(
    device_id: str,
    device_name: Optional[str] = Query(None),
    device_type: Optional[str] = Query(None),
    location: Optional[str] = Query(None),
    status: Optional[str] = Query(None),
    remarks: Optional[str] = Query(None),
):
    """UPDATE: 更新设备信息（支持部分更新）"""
    kwargs = {}
    if device_name is not None: kwargs["device_name"] = device_name
    if device_type is not None: kwargs["device_type"] = device_type
    if location is not None:    kwargs["location"] = location
    if status is not None:      kwargs["status"] = status
    if remarks is not None:     kwargs["remarks"] = remarks

    ok = database.update_device(device_id, **kwargs)
    if ok:
        return {"code": 0, "message": "设备更新成功"}
    return {"code": -1, "message": "设备不存在或未做修改"}


# ----- DELETE: 删除设备 -----
@app.delete("/api/v1/devices/{device_id}")
async def api_delete_device(device_id: str):
    """DELETE: 删除设备（级联删除关联数据）"""
    ok = database.delete_device(device_id)
    if ok:
        return {"code": 0, "message": f"设备 {device_id} 及关联数据已删除"}
    return {"code": -1, "message": "设备不存在"}


# ===================================================================
#  表2: sensor_data CRUD API
# ===================================================================

# ----- CREATE: 手动插入数据 -----
@app.post("/api/v1/sensor-data")
async def api_insert_sensor_data(
    device_id: str = Query("roomone"),
    Temp: str = Query(""),
    Humi: str = Query(""),
    Lumi: str = Query(""),
    LampST: str = Query("OFF"),
    CondST: str = Query("OFF"),
    VentST: str = Query("OFF"),
    BuzzerST: str = Query("OFF"),
    Smoke: str = Query("0"),
    CO2: str = Query("400"),
):
    """CREATE: 手动插入传感器数据"""
    data = {
        "device_id": device_id,
        "Temp": Temp, "Humi": Humi, "Lumi": Lumi,
        "LampST": LampST, "CondST": CondST, "VentST": VentST,
        "BuzzerST": BuzzerST, "Smoke": Smoke, "CO2": CO2,
    }
    record_id = database.insert_sensor_data(data)
    return {"code": 0, "data": {"id": record_id}, "message": "数据插入成功"}


# ----- READ: 最新数据 -----
@app.get("/api/v1/latest")
async def get_latest(device_id: Optional[str] = Query(None)):
    """READ: 获取最新数据"""
    data = get_latest_data()
    if data:
        return {"code": 0, "data": data, "message": "success"}
    rows = database.query_latest(1, device_id)
    if rows:
        return {"code": 0, "data": rows[0], "message": "success"}
    return {"code": 0, "data": None, "message": "暂无数据"}


# ----- READ: 历史数据 -----
@app.get("/api/v1/history")
async def get_history(
    limit: int = Query(100, ge=1, le=1000),
    offset: int = Query(0, ge=0),
    device_id: Optional[str] = Query(None),
    start_time: Optional[str] = Query(None),
    end_time: Optional[str] = Query(None),
):
    """READ: 查询历史数据"""
    rows = database.query_history(
        limit=limit, offset=offset, device_id=device_id,
        start_time=start_time, end_time=end_time,
    )
    return {"code": 0, "data": rows, "total": len(rows), "message": "success"}


# ----- UPDATE: 修改传感器数据 -----
@app.put("/api/v1/sensor-data/{record_id}")
async def api_update_sensor_data(record_id: int, **kwargs):
    """UPDATE: 修改传感器数据"""
    # 从查询参数中提取可修改字段
    update_fields = {}
    for field in ["Temp", "Humi", "Lumi", "LampST", "CondST", "VentST", "BuzzerST", "Smoke", "CO2"]:
        val = kwargs.get(field)
        if val is not None:
            update_fields[field] = val

    ok = database.update_sensor_data(record_id, **update_fields)
    if ok:
        return {"code": 0, "message": "数据更新成功"}
    return {"code": -1, "message": "记录不存在或未做修改"}


# ----- DELETE: 删除单条 -----
@app.delete("/api/v1/sensor-data/{record_id}")
async def api_delete_sensor_data(record_id: int):
    """DELETE: 删除指定传感器数据"""
    ok = database.delete_sensor_data(record_id)
    if ok:
        return {"code": 0, "message": "记录已删除"}
    return {"code": -1, "message": "记录不存在"}


# ----- DELETE: 批量删除 -----
@app.delete("/api/v1/sensor-data")
async def api_delete_sensor_data_batch(
    device_id: Optional[str] = Query(None),
    before_time: Optional[str] = Query(None),
):
    """DELETE: 批量删除（按设备或按时间）"""
    count = database.delete_sensor_data_batch(device_id, before_time)
    return {"code": 0, "data": {"deleted": count}, "message": f"已删除 {count} 条记录"}


# ===================================================================
#  表3: device_commands CRUD API
# ===================================================================

# ----- CREATE: 插入命令 -----
@app.post("/api/v1/commands")
async def api_insert_command(
    device_id: str = Query("roomone"),
    command: str = Query(..., description="命令名称"),
    param_key: str = Query(""),
    param_value: str = Query(""),
):
    """CREATE: 插入设备命令记录"""
    cmd_id = database.insert_command_log(device_id, command, param_key, param_value)
    return {"code": 0, "data": {"id": cmd_id}, "message": "命令记录已创建"}


# ----- READ: 查询命令日志 -----
@app.get("/api/v1/commands")
async def api_get_commands(
    limit: int = Query(50, ge=1, le=200),
    device_id: Optional[str] = Query(None),
):
    """READ: 查询命令日志"""
    rows = database.query_command_log(limit=limit, device_id=device_id)
    return {"code": 0, "data": rows, "total": len(rows), "message": "success"}


# ----- UPDATE: 更新命令状态 -----
@app.put("/api/v1/commands/{command_id}")
async def api_update_command(command_id: int, status: str = Query(...)):
    """UPDATE: 更新命令执行状态"""
    ok = database.update_command_status(command_id, status)
    if ok:
        return {"code": 0, "message": "命令状态已更新"}
    return {"code": -1, "message": "命令记录不存在"}


# ----- DELETE: 删除命令 -----
@app.delete("/api/v1/commands/{command_id}")
async def api_delete_command(command_id: int):
    """DELETE: 删除命令记录"""
    ok = database.delete_command(command_id)
    if ok:
        return {"code": 0, "message": "命令记录已删除"}
    return {"code": -1, "message": "命令记录不存在"}


# ===================================================================
#  统计与健康检查
# ===================================================================

@app.get("/api/v1/statistics")
async def get_statistics():
    """统计数据"""
    stats = database.get_statistics()
    return {"code": 0, "data": stats, "message": "success"}


@app.get("/api/v1/health")
async def health_check():
    """健康检查"""
    stats = database.get_statistics()
    return {
        "code": 0,
        "data": {
            "status": "running",
            "db_type": config.DB_TYPE,
            "data_source": config.DATA_SOURCE,
            "total_records": stats["total_records"],
            "total_devices": stats["total_devices"],
            "total_commands": stats["total_commands"],
        },
        "message": "success",
    }


# ===================================================================
#  WebSocket 实时推送
# ===================================================================

@app.websocket("/ws/realtime")
async def websocket_realtime(websocket: WebSocket):
    await websocket.accept()
    websocket_clients.add(websocket)
    logger.info(f"WebSocket 客户端已连接，当前连接数: {len(websocket_clients)}")
    try:
        while True:
            try:
                data = await websocket.receive_text()
                if data == "ping":
                    await websocket.send_json({"type": "pong"})
            except WebSocketDisconnect:
                break
    except Exception as e:
        logger.error(f"WebSocket 异常: {e}")
    finally:
        websocket_clients.discard(websocket)
        logger.info(f"WebSocket 客户端已断开，当前连接数: {len(websocket_clients)}")



# ===================================================================
#  AI 助手接口（对接 Dify 大模型平台）
# ===================================================================

class AiChatRequest(BaseModel):
    question: str


@app.post("/api/v1/ai/chat")
async def ai_chat(req: AiChatRequest):
    """AI 助手：将用户问题转发给 Dify，返回 AI 回答"""
    try:
        headers = {
            "Authorization": f"Bearer {config.DIFY_API_KEY}",
            "Content-Type": "application/json"
        }

        payload = {
            "inputs": {},
            "query": req.question,
            "response_mode": "blocking",
            "conversation_id": "",
            "user": "smart-mushroom-web"
        }

        resp = requests.post(
            config.DIFY_API_URL,
            headers=headers,
            json=payload,
            timeout=60
        )
        resp.raise_for_status()
        data = resp.json()

        return {
            "code": 0,
            "message": "success",
            "data": {
                "answer": data.get("answer", ""),
                "raw": data
            }
        }

    except Exception as e:
        logger.error(f"AI 助手调用失败: {e}")
        return {
            "code": -1,
            "message": str(e),
            "data": None
        }

# ===================================================================
#  程序入口
# ===================================================================

if __name__ == "__main__":
    uvicorn.run(
        "app:app",
        host=config.HTTP_HOST,
        port=config.HTTP_PORT,
        reload=False,
        log_level="info",
    )
