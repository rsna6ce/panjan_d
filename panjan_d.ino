#include <stdint.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include "mycon.h"
#include <esp_task_wdt.h>
#include "wps_example.h"
#include "SPIFFSIni.h"

static MyconReceiver MyconRecv;

const int pin_l_f = 32;
const int pin_l_b = 33;
const int pin_r_f = 26;
const int pin_r_b = 27;
const int pin_stby = 25;
const int pinLED = 2;

const int pwm_ch_r_f = 1;
const int pwm_ch_r_b = 2;
const int pwm_ch_l_f = 3;
const int pwm_ch_l_b = 4;
const int pwm_freq = 30;
const int pwm_bit = 8;
const int pwm_max = (1 << pwm_bit);
static int pwm_run_percent = 100;
static int pwm_turn_percent = 50;
static int pwm_spin_percent = 50;

WebServer server(80);
static String current_ipaddr = "";

//3 seconds WDT
#define WDT_TIMEOUT 3

const char* ssid = "";
const char* password = "";
#define WIFI_TIMEOUT 8

static int led_blink=0;
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("hello. panjan_d.");

    // setup pins
    pinMode(pin_l_f, OUTPUT);
    pinMode(pin_l_b, OUTPUT);
    pinMode(pin_r_f, OUTPUT);
    pinMode(pin_r_b, OUTPUT);
    pinMode(pin_stby, OUTPUT);
    digitalWrite(pin_l_f, LOW);
    digitalWrite(pin_l_b, LOW);
    digitalWrite(pin_r_f, LOW);
    digitalWrite(pin_r_b, LOW);
    digitalWrite(pin_stby, LOW);
    // setup pwm
    ledcSetup(pwm_ch_l_f, pwm_freq, pwm_bit);
    ledcSetup(pwm_ch_l_b, pwm_freq, pwm_bit);
    ledcSetup(pwm_ch_r_f, pwm_freq, pwm_bit);
    ledcSetup(pwm_ch_r_b, pwm_freq, pwm_bit);
    ledcAttachPin(pin_l_f, pwm_ch_l_f);
    ledcAttachPin(pin_l_b, pwm_ch_l_b);
    ledcAttachPin(pin_r_f, pwm_ch_r_f);
    ledcAttachPin(pin_r_b, pwm_ch_r_b);
    ledcWrite(pwm_ch_l_f, 0);
    ledcWrite(pwm_ch_l_b, 0);
    ledcWrite(pwm_ch_r_f, 0);
    ledcWrite(pwm_ch_r_b, 0);
    digitalWrite(pin_stby, HIGH);

    delay(1*1000);
    Serial.println("WiFi.begin");
    if (strlen(ssid)>0 && strlen(password)>0) {
        // specific ssid
        WiFi.begin(ssid, password);
    } else {
        // ssid last connected
        WiFi.begin();
    }
    int wifi_status = WL_DISCONNECTED;
    pinMode(0, INPUT_PULLUP);
    if (digitalRead(0) == LOW) {
        Serial.println("WiFi canceled");
        delay(1*1000);
    } else {
        for (int i=0; (i<WIFI_TIMEOUT*2)&&(wifi_status!= WL_CONNECTED); i++) {
            Serial.print(".");
            wifi_status = WiFi.status();
            digitalWrite(pinLED, led_blink);
            led_blink ^= 1;
            delay(500);
        }
    }
    digitalWrite(pinLED, LOW);
    if (wifi_status == WL_CONNECTED) {
        delay(1*1000);
        Serial.println("wifi connected.");
        current_ipaddr = WiFi.localIP().toString();
        Serial.println(current_ipaddr);
        Serial.println(WiFi.macAddress());
        digitalWrite(pinLED, HIGH);
        delay(1*1000);
    } else {
        // try WPS connection
        Serial.println("Failed to connect");
        Serial.println("Starting WPS");
        delay(1*1000);
        WiFi.onEvent(WiFiEvent);
        WiFi.mode(WIFI_MODE_STA);
        wpsInitConfig();
        wpsStart();
        for (int i=0; (i<WIFI_TIMEOUT/2)&&(!wps_success); i++) {
            Serial.print(".");
            // wps_success is updated in callback
            digitalWrite(pinLED, led_blink);
            led_blink ^= 1;
            delay(2000);
        }
        delay(2*1000);
        digitalWrite(pinLED, LOW);
        esp_restart();
    }
    MyconRecv.begin(MYCON_UDP_PORT);
    Serial.println("start finished");
    delay(100);

    // load config
    Serial.println("loading config.ini ...");
    SPIFFSIni config("/config.ini", true);
    if (config.exist("run")) {
        pwm_run_percent = config.read("run").toInt();
    }
    if (config.exist("turn")) {
        pwm_turn_percent = config.read("turn").toInt();
    }
    if (config.exist("spin")) {
        pwm_spin_percent = config.read("spin").toInt();
    }
    Serial.println("pwm_run_percent:" + String(pwm_run_percent));
    Serial.println("pwm_turn_percent:" + String(pwm_turn_percent));
    Serial.println("pwm_spin_percent:" + String(pwm_spin_percent));

    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/api",handleApi);
    server.onNotFound(handleNotFound);
    server.begin();

    //enable panic so ESP32 restarts
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL);
}

void handleRoot() {
    #include "index.html.h"
    index_html.replace("{{CURRENT_IPADDR}}", current_ipaddr);
    server.send(200, "text/HTML", index_html);
}

void handleConfig() {
    #include "config.html.h"
    config_html.replace("{{CURRENT_IPADDR}}", current_ipaddr);
    config_html.replace("{{RUN_VALUE}}", String(pwm_run_percent));
    config_html.replace("{{TURN_VALUE}}", String(pwm_turn_percent));
    config_html.replace("{{SPIN_VALUE}}", String(pwm_spin_percent));
    server.send(200, "text/HTML", config_html);
}

static String input_webapi = "";
void handleApi() {
    String ev_str = server.arg("ev");
    String res = "ERROR: invalid command.";
    if (ev_str == "forward" || ev_str == "forward_left" || ev_str == "forward_right" ||
        ev_str == "left" || ev_str == "stop" || ev_str == "right" ||
        ev_str == "backward" || ev_str == "backward_left" || ev_str == "backward_right") {
        res = "OK";
        input_webapi = ev_str;
    } else if(ev_str == "config") {
        String name_str = server.arg("name");
        String val_str = server.arg("val");
        SPIFFSIni config("/config.ini");
        if (name_str == "run") {
            pwm_run_percent = val_str.toInt();
            res = "OK run[%] = " + String(pwm_run_percent);
            config.write("run", String(pwm_run_percent));
        } else if (name_str == "turn") {
            pwm_turn_percent = val_str.toInt();
            res = "OK turn[%] = " + String(pwm_turn_percent);
            config.write("turn", String(pwm_turn_percent));
        } else if (name_str == "spin") {
            pwm_spin_percent = val_str.toInt();
            res = "OK spin[%] = " + String(pwm_spin_percent);
            config.write("spin", String(pwm_spin_percent));
        }
    } else {
        input_webapi = "stop";
        res = "OK";
    }
    Serial.println(input_webapi);
    server.send(200, "text/HTML", res);
}

bool is_key_down_webapi(String command) {
    return (input_webapi == command);
}

void handleNotFound() {
  server.send(404, "text/plain", "404 page not found.");
}

void motor_output(int output_l_f, int output_l_b, int output_r_f, int output_r_b) {
    //debug
    //Serial.println(String(output_l_f) + "," + String(output_l_b) + "," + String(output_r_f) + "," + String(output_r_b));
    //write
    ledcWrite(pwm_ch_l_f, (pwm_max * output_l_f) / 100);
    ledcWrite(pwm_ch_l_b, (pwm_max * output_l_b) / 100);
    ledcWrite(pwm_ch_r_f, (pwm_max * output_r_f) / 100);
    ledcWrite(pwm_ch_r_b, (pwm_max * output_r_b) / 100);
}

void loop() {
    // web i/f
    if (WiFi.status()==WL_CONNECTED) {
        server.handleClient();
    } else {
        Serial.println("ERROR: wifi disconnected!! restart...");
        esp_restart();
    }
    
    // input check
    bool input_forward = ( MyconRecv.is_key_down(key_Upward) |
        is_key_down_webapi("forward") |
        is_key_down_webapi("forward_left") |
        is_key_down_webapi("forward_right"));
    bool input_backward = (MyconRecv.is_key_down(key_Downward) |
        is_key_down_webapi("backward") |
        is_key_down_webapi("backward_right") |
        is_key_down_webapi("backward_left")) & (!input_forward);
    bool input_left = (MyconRecv.is_key_down(key_Left) |
        is_key_down_webapi("left") |
        is_key_down_webapi("forward_left") |
        is_key_down_webapi("backward_left"));
    bool input_right = (MyconRecv.is_key_down(key_Right) |
        is_key_down_webapi("right") |
        is_key_down_webapi("forward_right") |
        is_key_down_webapi("backward_right")) & (!input_left);

    // calc speed
    if (input_forward && input_left) {
        // forward turn left
        //Serial.println("forward turn left");
        motor_output(
            pwm_turn_percent,
            0,
            pwm_run_percent,
            0);
    } else if (input_forward && input_right) {
        // forward turn right
        //Serial.println("forward turn right");
        motor_output(
            pwm_run_percent,
            0,
            pwm_turn_percent,
            0);
    } else if (input_forward){
        // forward
        //Serial.println("forward");
        motor_output(
            pwm_run_percent,
            0,
            pwm_run_percent,
            0);
    } else if (input_backward && input_left) {
        // backward turn left
        //Serial.println("backward turn left");
        motor_output(
            0,
            pwm_turn_percent,
            0,
            pwm_run_percent);
    } else if (input_backward && input_right) {
        // backwardturn right
        //Serial.println("backwardturn right");
        motor_output(
            0,
            pwm_run_percent,
            pwm_turn_percent,
            0);
    } else if (input_backward){
        // backward
        //Serial.println("backward");
        motor_output(
            0,
            pwm_run_percent,
            0,
            pwm_run_percent);
    } else if (input_left) {
        // spin left
        //Serial.println("spin left");
        motor_output(
            0,
            pwm_spin_percent,
            pwm_spin_percent,
            0);
    } else if (input_right) {
        // spin right
        //Serial.println("spin right");
        motor_output(
            pwm_spin_percent,
            0,
            0,
            pwm_spin_percent);
    } else {
        // stop
        //Serial.println("stop");
        motor_output(0, 0, 0, 0);
    }

    esp_task_wdt_reset();
    delay(3);
}
