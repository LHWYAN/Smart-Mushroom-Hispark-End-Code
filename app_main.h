#ifndef _MAIN_H
#define _MAIN_H



// MQTT服务器配置（保持原有）
#define SERVER_IP_ADDR          "412aa055e1.st1.iotda-device.cn-north-4.myhuaweicloud.com"
#define SERVER_IP_PORT           1883


// MQTT主题配置（保持原有）
#define MQTT_CMDTOPIC_SUB       "$oc/devices/6a3a6e0d7f2e6c302f7e2ac9_roomone/sys/commands/#"
#define MQTT_DATATOPIC_PUB      "$oc/devices/6a3a6e0d7f2e6c302f7e2ac9_roomone/sys/properties/report"
#define MQTT_CLIENT_RESPONSE    "$oc/devices/6a3a6e0d7f2e6c302f7e2ac9_roomone/sys/commands/response/request_id=%s"

#define IOT
// 认证信息（保持原有）
#ifdef IOT
#define CLIENT_ID               "6a3a6e0d7f2e6c302f7e2ac9_roomone_0_0_2026062312"
#define DEVICEID                "6a3a6e0d7f2e6c302f7e2ac9_roomone"
#define CLIENTPASSWORD          "c4083010cded920e7cb5c928f2ffcf0bcb214b93c54423bb80b891ef81247e2a"
#endif


#define CONFIG_WIFI_SSID        "LHWYAN"      // 要连接的WiFi 热点账号
#define CONFIG_WIFI_PWD         "k64RS5rr"        // 要连接的WiFi 热点密码

#endif
