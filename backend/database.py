"""
==========================================================================
                    智慧房间 - 数据库模型与操作层
                    版权所有：沈阳市网联通信规划设计有限公司
==========================================================================
【数据库设计 - 3表关系模型】

┌──────────────────┐       ┌──────────────────────┐
│   device_info    │       │    sensor_data        │
│  (设备信息表)     │◄──────│  (传感器数据表)       │
│                  │  1:N  │                      │
│  device_id (PK)  │──────►│  device_id (FK)      │
│  device_name     │       │  Temp, Humi, Lumi... │
│  device_type     │       │  create_time          │
│  location        │       └──────────────────────┘
│  status          │
└────────┬─────────┘
         │ 1:N
         ▼
┌──────────────────────┐
│   device_commands    │
│  (设备命令表)        │
│                      │
│  device_id (FK)     │
│  command_name       │
│  param_key/value    │
│  status             │
│  create_time        │
└──────────────────────┘

支持 SQLite 和 KingbaseES（人大金仓）两种数据库。
==========================================================================
"""

import json
import logging
from datetime import datetime, timezone, timedelta
from typing import Optional

import config

logger = logging.getLogger(__name__)

BJT = timezone(timedelta(hours=8))

# SQL 占位符: SQLite 用 ?，KingbaseES(PostgreSQL) 用 %s
PH = "%s" if config.DB_TYPE == "kingbasees" else "?"


# ======================= 数据库连接 =======================

def get_connection():
    """获取数据库连接，自动选择 SQLite 或 KingbaseES"""
    if config.DB_TYPE == "kingbasees":
        return _get_kingbase_conn()
    return _get_sqlite_conn()


def _get_sqlite_conn():
    import sqlite3
    conn = sqlite3.connect(config.SQLITE_DB_PATH)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA foreign_keys=ON")  # 启用外键约束
    return conn


def _get_kingbase_conn():
    try:
        import psycopg2
        from psycopg2.extras import RealDictCursor
        conn = psycopg2.connect(
            host=config.KINGBASE_HOST,
            port=config.KINGBASE_PORT,
            dbname=config.KINGBASE_DB,
            user=config.KINGBASE_USER,
            password=config.KINGBASE_PASSWORD,
            cursor_factory=RealDictCursor,
        )
        return conn
    except ImportError:
        logger.error("psycopg2 未安装，请执行: pip install psycopg2-binary")
        raise
    except Exception as e:
        logger.error(f"KingbaseES 连接失败: {e}")
        raise


# ======================= 表结构 =======================
# 根据 DB_TYPE 选择不同的 SQL 语法

if config.DB_TYPE == "kingbasees":
    # ===== KingbaseES (PostgreSQL 兼容) =====
    CREATE_DEVICE_INFO_SQL = """
    CREATE TABLE IF NOT EXISTS device_info (
        id          BIGSERIAL PRIMARY KEY,
        device_id   VARCHAR(128) NOT NULL UNIQUE,
        device_name VARCHAR(256) DEFAULT '',
        device_type VARCHAR(64)  DEFAULT 'sensor',
        location    VARCHAR(256) DEFAULT '',
        status      VARCHAR(32)  DEFAULT 'offline',
        remarks     TEXT DEFAULT '',
        create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
    """
    CREATE_SENSOR_DATA_SQL = """
    CREATE TABLE IF NOT EXISTS sensor_data (
        id          BIGSERIAL PRIMARY KEY,
        device_id   VARCHAR(128) NOT NULL DEFAULT 'roomone',
        Temp        VARCHAR(32)  DEFAULT '',
        Humi        VARCHAR(32)  DEFAULT '',
        Lumi        VARCHAR(32)  DEFAULT '',
        LampST      VARCHAR(16)  DEFAULT 'OFF',
        CondST      VARCHAR(16)  DEFAULT 'OFF',
        VentST      VARCHAR(16)  DEFAULT 'OFF',
        BuzzerST    VARCHAR(16)  DEFAULT 'OFF',
        Smoke       VARCHAR(32)  DEFAULT '0',
        CO2         VARCHAR(32)  DEFAULT '400',
        create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        CONSTRAINT fk_sensor_device FOREIGN KEY (device_id)
            REFERENCES device_info(device_id) ON DELETE CASCADE
    );
    """
    CREATE_SENSOR_DATA_IDX_SQL = "CREATE INDEX IF NOT EXISTS idx_sensor_data_device ON sensor_data (device_id, create_time DESC);"
    CREATE_DEVICE_COMMANDS_SQL = """
    CREATE TABLE IF NOT EXISTS device_commands (
        id          BIGSERIAL PRIMARY KEY,
        device_id   VARCHAR(128) NOT NULL DEFAULT 'roomone',
        command     VARCHAR(128) NOT NULL,
        param_key   VARCHAR(128) DEFAULT '',
        param_value VARCHAR(512) DEFAULT '',
        status      VARCHAR(32)  DEFAULT 'pending',
        create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        CONSTRAINT fk_cmd_device FOREIGN KEY (device_id)
            REFERENCES device_info(device_id) ON DELETE CASCADE
    );
    """
    CREATE_COMMANDS_IDX_SQL = "CREATE INDEX IF NOT EXISTS idx_commands_device ON device_commands (device_id, create_time DESC);"
    SEED_DEVICE_SQL = """
    INSERT INTO device_info (device_id, device_name, device_type, location, status)
    VALUES ('roomone', '一号房间', 'sensor', '沈阳网联实验室', 'offline')
    ON CONFLICT (device_id) DO NOTHING;
    """
else:
    # ===== SQLite =====
    CREATE_DEVICE_INFO_SQL = """
    CREATE TABLE IF NOT EXISTS device_info (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id   TEXT NOT NULL UNIQUE,
        device_name TEXT NOT NULL DEFAULT '',
        device_type TEXT NOT NULL DEFAULT 'sensor',
        location    TEXT NOT NULL DEFAULT '',
        status      TEXT NOT NULL DEFAULT 'offline',
        remarks     TEXT NOT NULL DEFAULT '',
        create_time TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
        update_time TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
    );
    """
    CREATE_SENSOR_DATA_SQL = """
    CREATE TABLE IF NOT EXISTS sensor_data (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id   TEXT NOT NULL DEFAULT 'roomone',
        Temp        TEXT NOT NULL DEFAULT '',
        Humi        TEXT NOT NULL DEFAULT '',
        Lumi        TEXT NOT NULL DEFAULT '',
        LampST      TEXT NOT NULL DEFAULT 'OFF',
        CondST      TEXT NOT NULL DEFAULT 'OFF',
        VentST      TEXT NOT NULL DEFAULT 'OFF',
        BuzzerST    TEXT NOT NULL DEFAULT 'OFF',
        Smoke       TEXT NOT NULL DEFAULT '0',
        CO2         TEXT NOT NULL DEFAULT '400',
        create_time TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
        FOREIGN KEY (device_id) REFERENCES device_info(device_id)
    );
    """
    CREATE_SENSOR_DATA_IDX_SQL = "CREATE INDEX IF NOT EXISTS idx_sensor_data_device ON sensor_data (device_id, create_time DESC);"
    CREATE_DEVICE_COMMANDS_SQL = """
    CREATE TABLE IF NOT EXISTS device_commands (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id   TEXT NOT NULL DEFAULT 'roomone',
        command     TEXT NOT NULL,
        param_key   TEXT NOT NULL DEFAULT '',
        param_value TEXT NOT NULL DEFAULT '',
        status      TEXT NOT NULL DEFAULT 'pending',
        create_time TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
        update_time TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
        FOREIGN KEY (device_id) REFERENCES device_info(device_id)
    );
    """
    CREATE_COMMANDS_IDX_SQL = "CREATE INDEX IF NOT EXISTS idx_commands_device ON device_commands (device_id, create_time DESC);"
    SEED_DEVICE_SQL = """
    INSERT OR IGNORE INTO device_info (device_id, device_name, device_type, location, status)
    VALUES ('roomone', '一号房间', 'sensor', '沈阳网联实验室', 'offline');
    """


def init_database():
    """初始化数据库表结构 + 种子数据"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(CREATE_DEVICE_INFO_SQL)
        cursor.execute(CREATE_SENSOR_DATA_SQL)
        cursor.execute(CREATE_SENSOR_DATA_IDX_SQL)
        cursor.execute(CREATE_DEVICE_COMMANDS_SQL)
        cursor.execute(CREATE_COMMANDS_IDX_SQL)
        cursor.execute(SEED_DEVICE_SQL)
        conn.commit()
        logger.info("数据库初始化完成 (3张表 + 默认设备)")
    except Exception as e:
        logger.error(f"数据库初始化失败: {e}")
        raise
    finally:
        conn.close()


# ======================= 📋 通用工具 =======================

def _parse_val(v):
    """安全转换数值"""
    if v is None or v == '':
        return 0
    try:
        return float(v)
    except (ValueError, TypeError):
        return 0


# ======================= 🔷 表1: device_info CRUD =======================

# -------- CREATE: 注册新设备 --------
def create_device(device_id: str, device_name: str = "", device_type: str = "sensor",
                  location: str = "", remarks: str = "") -> dict:
    """注册一台新设备，返回设备信息"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(
            f"""INSERT INTO device_info
               (device_id, device_name, device_type, location, status, remarks)
               VALUES ({PH}, {PH}, {PH}, {PH}, 'offline', {PH})
               RETURNING id""",
            (device_id, device_name, device_type, location, remarks),
        )
        new_id = cursor.fetchone()["id"]
        conn.commit()
        return {"id": new_id, "device_id": device_id, "message": "设备注册成功"}
    except Exception as e:
        logger.error(f"注册设备失败: {e}")
        raise
    finally:
        conn.close()


# -------- READ: 查询设备列表 --------
def get_all_devices() -> list:
    """查询所有设备"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM device_info ORDER BY id ASC")
        return [dict(row) for row in cursor.fetchall()]
    finally:
        conn.close()


def get_device_by_id(device_id: str) -> Optional[dict]:
    """按 device_id 查询单个设备"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(f"SELECT * FROM device_info WHERE device_id = {PH}", (device_id,))
        row = cursor.fetchone()
        return dict(row) if row else None
    finally:
        conn.close()


# -------- UPDATE: 更新设备信息 --------
def update_device(device_id: str, **kwargs) -> bool:
    """更新设备信息（支持部分更新）"""
    allowed_fields = {"device_name", "device_type", "location", "status", "remarks"}
    updates = {k: v for k, v in kwargs.items() if k in allowed_fields and v is not None}
    if not updates:
        return False

    set_clause = ", ".join(f"{k} = {PH}" for k in updates.keys())
    updates["update_time"] = datetime.now(BJT).strftime("%Y-%m-%d %H:%M:%S")

    sql = f"UPDATE device_info SET {set_clause}, update_time = {PH} WHERE device_id = {PH}"
    params = list(updates.values()) + [device_id]

    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(sql, params)
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()


# -------- DELETE: 删除设备 --------
def delete_device(device_id: str) -> bool:
    """删除设备（会级联删除关联的传感器数据和命令记录）"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        # 先删除关联数据
        cursor.execute(f"DELETE FROM sensor_data WHERE device_id = {PH}", (device_id,))
        cursor.execute(f"DELETE FROM device_commands WHERE device_id = {PH}", (device_id,))
        # 再删除设备
        cursor.execute(f"DELETE FROM device_info WHERE device_id = {PH}", (device_id,))
        conn.commit()
        return cursor.rowcount > 0
    except Exception as e:
        logger.error(f"删除设备失败: {e}")
        conn.rollback()
        return False
    finally:
        conn.close()


# ======================= 🔷 表2: sensor_data CRUD =======================

# -------- CREATE: 插入传感器数据 --------
def insert_sensor_data(data: dict) -> int:
    """插入一条传感器数据"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(
            f"""INSERT INTO sensor_data
               (device_id, Temp, Humi, Lumi, LampST, CondST, VentST, BuzzerST, Smoke, CO2)
               VALUES ({PH}, {PH}, {PH}, {PH}, {PH}, {PH}, {PH}, {PH}, {PH}, {PH})
               RETURNING id""",
            (
                str(data.get("device_id", "roomone")),
                str(data.get("Temp", "")),
                str(data.get("Humi", "")),
                str(data.get("Lumi", "")),
                str(data.get("LampST", "OFF")),
                str(data.get("CondST", "OFF")),
                str(data.get("VentST", "OFF")),
                str(data.get("BuzzerST", "OFF")),
                str(data.get("Smoke", "0")),
                str(data.get("CO2", "400")),
            ),
        )
        new_id = cursor.fetchone()["id"]
        conn.commit()
        return new_id
    finally:
        conn.close()


# -------- READ: 查询传感器数据 --------
def query_latest(count: int = 1, device_id: Optional[str] = None) -> list:
    """查询最新 N 条数据，可按设备过滤"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        if device_id:
            cursor.execute(
                f"SELECT * FROM sensor_data WHERE device_id = {PH} ORDER BY id DESC LIMIT {PH}",
                (device_id, count),
            )
        else:
            cursor.execute(
                f"SELECT * FROM sensor_data ORDER BY id DESC LIMIT {PH}", (count,)
            )
        return [dict(row) for row in cursor.fetchall()]
    finally:
        conn.close()


def query_history(limit: int = 100, offset: int = 0, device_id: Optional[str] = None,
                  start_time: Optional[str] = None, end_time: Optional[str] = None) -> list:
    """查询历史数据，支持多条件过滤"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        conditions = []
        params = []

        if device_id:
            conditions.append(f"device_id = {PH}")
            params.append(device_id)
        if start_time:
            conditions.append(f"create_time >= {PH}")
            params.append(start_time)
        if end_time:
            conditions.append(f"create_time <= {PH}")
            params.append(end_time)

        where_clause = ""
        if conditions:
            where_clause = "WHERE " + " AND ".join(conditions)

        sql = f"SELECT * FROM sensor_data {where_clause} ORDER BY id DESC LIMIT {PH} OFFSET {PH}"
        params.extend([limit, offset])
        cursor.execute(sql, params)
        return [dict(row) for row in cursor.fetchall()]
    finally:
        conn.close()


# -------- UPDATE: 修改传感器数据 --------
def update_sensor_data(record_id: int, **kwargs) -> bool:
    """修改指定传感器数据记录"""
    allowed = {"Temp", "Humi", "Lumi", "LampST", "CondST", "VentST", "BuzzerST", "Smoke", "CO2"}
    updates = {k: v for k, v in kwargs.items() if k in allowed and v is not None}
    if not updates:
        return False

    set_clause = ", ".join(f"{k} = {PH}" for k in updates.keys())
    sql = f"UPDATE sensor_data SET {set_clause} WHERE id = {PH}"
    params = list(updates.values()) + [record_id]

    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(sql, params)
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()


# -------- DELETE: 删除传感器数据 --------
def delete_sensor_data(record_id: int) -> bool:
    """删除指定传感器数据记录"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(f"DELETE FROM sensor_data WHERE id = {PH}", (record_id,))
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()


def delete_sensor_data_batch(device_id: Optional[str] = None,
                             before_time: Optional[str] = None) -> int:
    """批量删除传感器数据"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        conditions = []
        params = []
        if device_id:
            conditions.append(f"device_id = {PH}")
            params.append(device_id)
        if before_time:
            conditions.append(f"create_time < {PH}")
            params.append(before_time)

        where_clause = ""
        if conditions:
            where_clause = "WHERE " + " AND ".join(conditions)

        cursor.execute(f"DELETE FROM sensor_data {where_clause}", params)
        conn.commit()
        return cursor.rowcount
    finally:
        conn.close()


# ======================= 🔷 表3: device_commands CRUD =======================

# -------- CREATE: 插入命令记录 --------
def insert_command_log(device_id: str, command: str, param_key: str = "",
                       param_value: str = "", status: str = "pending") -> int:
    """插入一条命令记录"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(
            f"""INSERT INTO device_commands (device_id, command, param_key, param_value, status)
               VALUES ({PH}, {PH}, {PH}, {PH}, {PH})
               RETURNING id""",
            (device_id, command, param_key, param_value, status),
        )
        new_id = cursor.fetchone()["id"]
        conn.commit()
        return new_id
    finally:
        conn.close()


# -------- READ: 查询命令记录 --------
def query_command_log(limit: int = 50, device_id: Optional[str] = None) -> list:
    """查询命令日志，可按设备过滤"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        if device_id:
            cursor.execute(
                f"SELECT * FROM device_commands WHERE device_id = {PH} ORDER BY id DESC LIMIT {PH}",
                (device_id, limit),
            )
        else:
            cursor.execute(
                f"SELECT * FROM device_commands ORDER BY id DESC LIMIT {PH}", (limit,)
            )
        return [dict(row) for row in cursor.fetchall()]
    finally:
        conn.close()


# -------- UPDATE: 更新命令执行结果 --------
def update_command_status(command_id: int, status: str) -> bool:
    """更新命令执行状态"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        now = datetime.now(BJT).strftime("%Y-%m-%d %H:%M:%S")
        cursor.execute(
            f"UPDATE device_commands SET status = {PH}, update_time = {PH} WHERE id = {PH}",
            (status, now, command_id),
        )
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()


# -------- DELETE: 删除命令记录 --------
def delete_command(command_id: int) -> bool:
    """删除指定命令记录"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(f"DELETE FROM device_commands WHERE id = {PH}", (command_id,))
        conn.commit()
        return cursor.rowcount > 0
    finally:
        conn.close()


# ======================= 📊 统计查询 =======================

def get_statistics() -> dict:
    """获取全局统计数据"""
    conn = get_connection()
    try:
        cursor = conn.cursor()
        cursor.execute("SELECT COUNT(*) as total FROM sensor_data")
        sensor_total = cursor.fetchone()["total"]

        cursor.execute("SELECT COUNT(*) as total FROM device_info")
        device_total = cursor.fetchone()["total"]

        cursor.execute("SELECT COUNT(*) as total FROM device_commands")
        cmd_total = cursor.fetchone()["total"]

        latest = query_latest(1)
        return {
            "total_records": sensor_total,
            "total_devices": device_total,
            "total_commands": cmd_total,
            "latest": latest[0] if latest else None,
        }
    finally:
        conn.close()