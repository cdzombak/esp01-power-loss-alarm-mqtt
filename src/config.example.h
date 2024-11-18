
#define CFG_HOSTNAME "power-loss-alarm-mqtt"
#define CFG_WIFI_ESSID "mywifi"
#define CFG_WIFI_PASSWORD "mywifipassword"

#define CFG_MQTT_HOST "mqtt.example.lan"
#define CFG_MQTT_PORT 1883
#define CFG_MQTT_USER "foobar"
#define CFG_MQTT_PASSWORD "foobarbaz"

#define CFG_MQTT_ALARM_TOPIC "home/power/alarms"
#define CFG_MQTT_STATE_TOPIC "home/power/power-loss-alarm-mqtt/ip"
#define CFG_MQTT_OTA_TOPIC "home/power/power-loss-alarm-mqtt/ota"

// the following work with https://github.com/cdzombak/mqttshutdownd :
#define PWR_LOSS_NOTIF "{\"up\":false,\"type\":1,\"scope\":\"global\"}"
#define PWR_RESTORE_NOTIF "{\"up\":true,\"type\":1,\"scope\":\"global\"}"
