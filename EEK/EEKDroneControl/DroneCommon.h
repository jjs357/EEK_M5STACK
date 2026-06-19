/*
 *  This sketch sends Tello commands over UDP from a ESP32 device
 *
 */
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <MPU6050_light.h>
#include <Adafruit_SSD1306.h>
#include <Button2.h>

// LED configs Huzzah
#define LED_CONN_RED 26
#define IN_FLIGHT 25
#define LED_CONN_GREEN 21
#define LED_BATT_RED 27
#define LED_BATT_YELLOW 15
#define LED_BATT_GREEN 4
#define COMMAND_TICK 13
#define UP_PIN 34
#define TAKEOFF_PIN 33
#define CW_PIN 32
#define CCW_PIN 39
#define KILL_PIN 36
#define DOWN_PIN 14

#define VBATPIN 35
#define FORMAT_SPIFFS_IF_FAILED true

File flightFile;
const char *flightFilePath = "/flight_file.txt";

const float MAX_BATTERY_VOLTAGE = 4.2;  // Max LiPoly voltage of a 3.7 battery is 4.2

//IP address to send UDP data to:
// either use the ip address of the server or
// a network broadcast address
const char *udpAddress = "192.168.10.1";
const int udpPort = 8889;

MPU6050 mpu(Wire);  // Wire.h included by default for ESP32
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

Button2 takeoffButton, killButton;

int Roll;
int AbsRoll;
int Pitch;
int AbsPitch;
int Yaw;
int limitedPitch;
int limitedRoll;

String rcCmd = "rc 0 0 0 0";
String lastRcCmd = "rc 0 0 0 0";
String lastCommand;

int upState = 0;
int downState = 0;
int cwState = 0;
int ccwState = 0;

unsigned long last_since_takeoff = 0;
unsigned long this_since_takeoff = 0;
unsigned long takeoff_time = 0;
unsigned long commandDelay = 0;

//The udp library class
WiFiUDP udp;

//Are we currently connected?
boolean connected;
boolean in_flight = false;
boolean lastCommandOK = false;
long last_command_millis = 0;
boolean use_MPUtoFly = false;
boolean use_SDK_KeepAlive = true;

int disconnected_tick = 0;
uint8_t buffer[50];

Preferences preferences;

String saved_ssid;
