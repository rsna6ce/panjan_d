#pragma once
#include "esp_system.h"
#include <stdint.h>
#include <WiFiUdp.h>

#define MYCON_UDP_PORT 59630
#define MYCON_KEY_COUNT 16
#define MYCON_RECV_TIMEOUT 500

enum key_index  {
    key_Upward,
    key_Downward,
    key_Left,
    key_Right,
    key_A,
    key_B,
    key_C,
    key_D,
    key_L1,
    key_L2,
    key_R1,
    key_R2,
    key_Select,
    key_Start,
    key_1,
    key_2,
    key_heartbeat
};

static char key_letter[] = {'U','D','L','R', 'A','B','C','D', 'L','l','R','r', 'E','T', '1','2'};


class MyconReceiver {
    public:
        MyconReceiver(){};

    private:
        WiFiUDP _wifiUdp;
        int _udp_port;
        int _is_started;
        TaskHandle_t Task1;
        bool _keyDown[MYCON_KEY_COUNT]={0};
        bool _debug_output = false;

    public: void begin(int port) {
      this->_wifiUdp.begin(port);
      xTaskCreatePinnedToCore(this->loopTask, "mycon_recv_loop", 4096, this, 1, &Task1, 0);
    }

    public: bool is_key_down(int index) {
        bool ret = false;
        if (index < MYCON_KEY_COUNT) {
            ret = _keyDown[index];
        }
        return ret;
    }

    public: void set_debug_output(bool debug_output) {
        _debug_output = debug_output;
    }

    private: static void loopTask(void *pvParameters) {
        char packetBuffer[256]={};
        MyconReceiver *l_pThis = (MyconReceiver *) pvParameters;
        uint64_t millis_recv_last = 0;
        while(true) {
            int packetSize = l_pThis->_wifiUdp.parsePacket();
            if (packetSize) {
                l_pThis->_wifiUdp.read(packetBuffer, 256);
                IPAddress remoteIP = l_pThis->_wifiUdp.remoteIP();
                if (l_pThis->_debug_output) {
                    Serial.print(remoteIP);
                    Serial.print(" ");
                    Serial.println(packetBuffer);
                }
                if (packetBuffer[key_heartbeat]=='H') {
                    l_pThis->_wifiUdp.beginPacket(remoteIP, MYCON_UDP_PORT);
                    l_pThis->_wifiUdp.print("H");
                    l_pThis->_wifiUdp.endPacket();
                }
                for (int i=0; i<MYCON_KEY_COUNT; i++) {
                    l_pThis->_keyDown[i] = (packetBuffer[i] == key_letter[i]);
                }
                millis_recv_last = millis();
            } else {
                uint64_t millis_curr = millis();
                if (millis_curr - millis_recv_last > MYCON_RECV_TIMEOUT) {
                    for (int i=0; i<MYCON_KEY_COUNT; i++) {
                        l_pThis->_keyDown[i] = false;
                    }
                }
            }
            delay(10);
        }
    }
};

