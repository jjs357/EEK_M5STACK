/*
 *  This sketch sends Tello commands over UDP from a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <MPU6050_light.h>
#include <Adafruit_SSD1306.h>
#include <Button2.h>

#define Feather
// #define DevKit

#define LED_USING_MPU 26
#define IN_CONTROL 25
#define LED_BATT_RED 27
#define LED_BATT_YELLOW 15
#define LED_BATT_GREEN 4
#define COMMAND_TICK 26
#define UP_PIN 34
#define TAKEOFF_PIN 33
#define CW_PIN 32
#define CCW_PIN 39
#define KILL_PIN 36
#define DOWN_PIN 14

#ifdef Feather
#define LED_CONN_GREEN 21
#endif
#ifdef DevKit
#define LED_CONN_GREEN 16
#endif

//IP address to send UDP data to:
// either use the ip address of the server or
// a network broadcast address
const char *udpAddress = "192.168.10.1";
const int udpPort = 8889;

MPU6050 mpu(Wire);  // Wire.h included by default for ESP32
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

Button2 takeoffButton, killButton, upButton, downButton, cwButton, ccwButton;

//The udp library class
WiFiUDP udp;

int Roll;
int AbsRoll;
int Pitch;
int AbsPitch;
int limitedPitch;
int limitedRoll;

//Are we currently connected?
boolean connected;
boolean in_rotation = false;

boolean in_control = false; // If the StackChan servo angles are at the Home position
boolean lastCommandOK = false;
long last_command_millis = 0;
int disconnected_tick = 0;
uint8_t buffer[50];

boolean rotateXdirCW = true;
boolean use_MPUtoControl = false;

boolean cameraOn = true;

Preferences preferences;

String saved_ssid;

unsigned long rcTime = 0;
unsigned long rcTimerLimit = 1500;

String rcCmd = "stop";
String lastRcCmd = "stop";
String rcCmdHead = "move ";

int maxYAngle = 900;
int Yangle = 0;
int maxXAngle = 1280;
int Xangle = 0;

String lastCommand;
