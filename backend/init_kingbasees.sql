/*
==========================================================================
                    智慧房间 - KingbaseES(人大金仓) 数据库初始化脚本
                    版权所有：沈阳市网联通信规划设计有限公司
==========================================================================
【3表关系模型】
  device_info (1) ──1:N──► sensor_data (N)
  device_info (1) ──1:N──► device_commands (N)

使用方式：
    ksql -U system -d smart_room -f init_kingbasees.sql
==========================================================================
*/


-- ===== 创建数据库 =====
-- 先连接到一个已存在数据库后执行：
-- CREATE DATABASE smart_room ENCODING 'UTF8';
-- \c smart_room


-- ===== 表1: device_info (设备信息表) =====
-- 用途：管理所有IoT设备的注册信息
-- CRUD 演示：注册设备(C) / 查询设备(R) / 修改设备信息(U) / 移除设备(D)
CREATE TABLE IF NOT EXISTS device_info (
    id          SERIAL PRIMARY KEY,                          -- 自增主键
    device_id   VARCHAR(64) NOT NULL UNIQUE,                 -- 设备唯一标识 (如 "roomone")
    device_name VARCHAR(128) NOT NULL DEFAULT '',            -- 设备友好名称 (如 "一号房间")
    device_type VARCHAR(32) NOT NULL DEFAULT 'sensor',       -- 设备类型 (sensor/actuator/gateway)
    location    VARCHAR(256) NOT NULL DEFAULT '',            -- 安装位置
    status      VARCHAR(16) NOT NULL DEFAULT 'offline',      -- 在线状态 (online/offline)
    remarks     TEXT NOT NULL DEFAULT '',                    -- 备注信息
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, -- 创建时间
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP  -- 更新时间
);

-- 设备ID唯一索引
CREATE UNIQUE INDEX IF NOT EXISTS idx_device_info_id ON device_info (device_id);


-- ===== 表2: sensor_data (传感器数据表) =====
-- 用途：存储WS63设备上报的环境传感器数据，关联到具体设备
-- CRUD 演示：插入采集数据(C) / 查询历史(R) / 修正错误数据(U) / 清理旧数据(D)
CREATE TABLE IF NOT EXISTS sensor_data (
    id          SERIAL PRIMARY KEY,                          -- 自增主键
    device_id   VARCHAR(64) NOT NULL DEFAULT 'roomone',      -- 关联 device_info.device_id
    Temp        VARCHAR(20) NOT NULL DEFAULT '',             -- 温度 (如 "28.5")
    Humi        VARCHAR(20) NOT NULL DEFAULT '',             -- 湿度 (如 "60.2")
    Lumi        VARCHAR(20) NOT NULL DEFAULT '',             -- 光照强度 (如 "300")
    LampST      VARCHAR(10) NOT NULL DEFAULT 'OFF',          -- 照明灯状态
    CondST      VARCHAR(10) NOT NULL DEFAULT 'OFF',          -- 空调状态
    VentST      VARCHAR(10) NOT NULL DEFAULT 'OFF',          -- 排风扇状态
    BuzzerST    VARCHAR(10) NOT NULL DEFAULT 'OFF',          -- 蜂鸣器状态
    Smoke       VARCHAR(20) NOT NULL DEFAULT '0',            -- 烟雾浓度 (%)
    CO2         VARCHAR(20) NOT NULL DEFAULT '400',          -- 二氧化碳浓度 (ppm)
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, -- 记录时间
    -- 外键约束：确保 device_id 必须在 device_info 中存在
    FOREIGN KEY (device_id) REFERENCES device_info(device_id) ON DELETE CASCADE
);

-- 按设备和时间查询的联合索引
CREATE INDEX IF NOT EXISTS idx_sensor_data_device ON sensor_data (device_id, create_time DESC);


-- ===== 表3: device_commands (设备命令表) =====
-- 用途：记录云平台下发的所有设备控制命令及执行结果
-- CRUD 演示：下发命令(C) / 查询命令历史(R) / 更新执行状态(U) / 删除记录(D)
CREATE TABLE IF NOT EXISTS device_commands (
    id          SERIAL PRIMARY KEY,                          -- 自增主键
    device_id   VARCHAR(64) NOT NULL DEFAULT 'roomone',      -- 关联 device_info.device_id
    command     VARCHAR(64) NOT NULL,                        -- 命令名称 (SetLamp / SetCond / SetVent / SetBuzzer)
    param_key   VARCHAR(32) NOT NULL DEFAULT '',             -- 参数键 (如 LampStatus)
    param_value VARCHAR(32) NOT NULL DEFAULT '',             -- 参数值 (ON / OFF)
    status      VARCHAR(16) NOT NULL DEFAULT 'pending',      -- 执行状态 (pending/success/fail)
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, -- 命令创建时间
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, -- 状态更新时间
    -- 外键约束
    FOREIGN KEY (device_id) REFERENCES device_info(device_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_commands_device ON device_commands (device_id, create_time DESC);


-- ===== 插入种子数据（默认设备） =====
INSERT INTO device_info (device_id, device_name, device_type, location, status)
VALUES ('roomone', '一号房间', 'sensor', '沈阳网联实验室', 'offline')
ON CONFLICT (device_id) DO NOTHING;


-- ===== 验证表结构 =====
-- \dt
-- \d device_info
-- \d sensor_data
-- \d device_commands

-- ===== CRUD 操作示例 =====
/*
--- CREATE ---
INSERT INTO device_info (device_id, device_name, device_type, location)
VALUES ('roomtwo', '二号房间', 'sensor', '二楼实验室');

--- READ ---
SELECT * FROM device_info;
SELECT * FROM sensor_data WHERE device_id = 'roomone' ORDER BY create_time DESC LIMIT 10;

--- UPDATE ---
UPDATE device_info SET status = 'online', update_time = CURRENT_TIMESTAMP
WHERE device_id = 'roomone';

--- DELETE ---
DELETE FROM device_info WHERE device_id = 'roomtwo';
*/