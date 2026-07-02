/*
 *  This sketch sends Tello commands over UDP from a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <M5Unified.h>
#include <M5HatMiniJoyC.h>

// MiniJoyC I2C pins
#define MiniJoyC_SDA 0
#define MiniJoyC_SCL 26
#define COMMAND_TICK 10 // StickCPlus Red LED


float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;

float pitch = 0.0F;
float roll = 0.0F;

M5HatMiniJoyC joyc;

// Create a reference alias for the display and imu
auto& display = M5.Lcd;
auto& imu = M5.Imu;

//IP address to send UDP data to:
// either use the ip address of the server or
// a network broadcast address
const char *udpAddress = "192.168.10.1";
const int udpPort = 8889;

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

boolean use_IMUtoControl = true;
boolean rotateXdirCW = true;

boolean cameraOn = false;

Preferences preferences;

String saved_ssid;

int jsReady = 0;
int jsXangle = 0;
int jsYangle = 0;


unsigned long jsTime = 0;
unsigned long jsTimerLimit = 500;
unsigned long imuTime = 0;
unsigned long imuTimerLimit = 1000;

String rcCmd = "stop";
String lastRcCmd = "stop";
String rcCmdHead = "move ";

int maxYAngle = 900;
int Yangle = 0;
int maxXAngle = 1280;
int Xangle = 0;

String lastCommand;

int conn[3] = { 10, 140, 0 };
int battery[3] = { 10, 170, 0 };
int control[3] = { 10, 200, 0 };
int use_imu[3] = { 90, 200, 0 };

int LEDsTop = 120;
int ledSize = 19;
