#include <Arduino.h>
#include <ESP8266WiFi.h> // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#include "config.h"

const uint8_t ledPin = 1;
const uint8_t vccSensPin = 3;

// program state:

int vccSensState = 2;
int lastOTAProgress = -1;
unsigned long lastIpBroadcast = 0;

// WiFi & MQTT:

WiFiClient wfClient;
PubSubClient mqttClient(CFG_MQTT_HOST, CFG_MQTT_PORT, wfClient);

// forward declarations:

bool sendAlarm(const String& alarm);
bool sendOtaMsg(const String& msg);
void ledToggle();

// program core:

void setup() {
    pinMode(vccSensPin, INPUT);
    pinMode(ledPin, OUTPUT);
    ledToggle();

    WiFi.mode(WIFI_STA);
    WiFi.hostname(CFG_HOSTNAME);
    WiFi.begin(CFG_WIFI_ESSID, CFG_WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        yield();
        delay(500);
        ledToggle();
    }
    WiFi.setSleep(true);

    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(CFG_HOSTNAME);
    ArduinoOTA.setRebootOnSuccess(true);
    ArduinoOTA.onStart([]() {
        #ifdef CFG_MQTT_OTA_TOPIC
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_FS
            type = "filesystem";
        }
        sendOtaMsg("OTA Start: " + type);
        #endif
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        #ifdef CFG_MQTT_OTA_TOPIC
        int pct = progress / (total / 100);
        if (pct != lastOTAProgress && pct % 5 == 0) {
            lastOTAProgress = pct;
            sendOtaMsg("OTA Progress: " + String(pct) + "%");
        }
        #endif
    });
    ArduinoOTA.onError([](ota_error_t error) {
        #ifdef CFG_MQTT_OTA_TOPIC
        if (error == OTA_AUTH_ERROR) {
            sendOtaMsg("OTA: Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            sendOtaMsg("OTA: Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            sendOtaMsg("OTA: Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            sendOtaMsg("OTA: Receive Failed");
        } else if (error == OTA_END_ERROR) {
            sendOtaMsg("OTA: End Failed");
        } else {
            sendOtaMsg("OTA Error: " + String(error));
        }
        #endif
    });
    ArduinoOTA.begin();

    ledToggle();
    randomSeed(micros());
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    String clientId = "homemqtt-pwrloss-monitor-" + String(random(0xffff), HEX);
    #ifdef CFG_MQTT_PASSWORD
    if (!mqttClient.connect(clientId.c_str(), CFG_MQTT_USER, CFG_MQTT_PASSWORD)) {
    #else
    if (!mqttClient.connect(clientId.c_str())) {
    #endif
      delay(2500);
      ledToggle();
    }
  }
}

void loop() {
    ArduinoOTA.handle();

    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    mqttClient.loop();

    bool vccSensStateChanged = false;

    if (digitalRead(vccSensPin) == HIGH && vccSensState < 5) {
        if (vccSensState == 4) {
            vccSensStateChanged = true;
        }
        vccSensState++;
    } else if (digitalRead(vccSensPin) == LOW && vccSensState > 0) {
        if (vccSensState != 0) {
            vccSensStateChanged = true;
        }
        vccSensState--;
    }

    if (vccSensStateChanged && vccSensState == 0) {
        if (!sendAlarm(PWR_LOSS_NOTIF)) {
            vccSensState = 1;
        }
    } else if (vccSensStateChanged && vccSensState == 5) {
        sendAlarm(PWR_RESTORE_NOTIF);
    }

    if (millis() - lastIpBroadcast > 10000) {
        lastIpBroadcast = millis();
        mqttClient.publish(CFG_MQTT_STATE_TOPIC, WiFi.localIP().toString().c_str());
    }

    delay(100);
    ledToggle();
}

bool sendAlarm(const String& alarm) {
    return mqttClient.publish(CFG_MQTT_ALARM_TOPIC, alarm.c_str());
}

bool sendOtaMsg(const String& msg) {
    #ifdef CFG_MQTT_OTA_TOPIC
    return mqttClient.publish(CFG_MQTT_OTA_TOPIC, msg.c_str());
    #else
    return true;
    #endif
}

void ledToggle() {
    digitalWrite(ledPin, !digitalRead(ledPin));
}
