/* 
 * Joseph DeMarco Jim Solderitsch
 * 
 * Tello Drone Simulator Using Cheap Yellow Display
 *
*/
#include <WiFi.h>
#include <base64.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <M5StackChan.h>
#include "esp_camera.h"

M5GFX display;
M5Canvas tft(&display);     // Virtual Screen 1
M5Canvas camera(&display);  // Virtual Screen 2

String serialNumberBase = "CS3SC";

int up[3] = { 60, 10, 0 };
int down[3] = { 60, 70, 0 };
int ccw[3] = { 30, 40, 0 };
int cw[3] = { 90, 40, 0 };

int connected[3] = { 10, 100, 0 };
int control[3] = { 210, 100, 0 };
int battery[3] = { 110, 100, 0 };
int textTop = 150;
int ledSize = 19;

WiFiUDP Udp;  // Creation of wifi Udp instance

char packetBuffer[255];
char responseBuffer[255];

unsigned int localPort = 8889;
unsigned long lastCommandTime = 0;
bool cameraEnabled = false;

int batLevel = 100;
int yAnglePrev;
int commandMode = 0;
int speed = 100;

const char* ssid = "STACK-M5CJSO";

const char* password = "";

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

Preferences preferences;

String serialNumber = serialNumberBase + String(ssid);
String prefs_ssid;
String prefs_sn;

class GC0308 {
private:
public:
  camera_fb_t* fb;
  sensor_t* sensor;
  camera_config_t* config;
  bool begin();
  bool get();
  bool free();
};
static camera_config_t camera_config = {
  .pin_pwdn = -1,
  .pin_reset = -1,
  .pin_xclk = -1,
  .pin_sscb_sda = 12,
  .pin_sscb_scl = 11,
  .pin_d7 = 47,
  .pin_d6 = 48,
  .pin_d5 = 16,
  .pin_d4 = 15,
  .pin_d3 = 42,
  .pin_d2 = 41,
  .pin_d1 = 40,
  .pin_d0 = 39,
  .pin_vsync = 46,
  .pin_href = 38,
  .pin_pclk = 45,
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_RGB565,
  .frame_size = FRAMESIZE_QVGA,
  .jpeg_quality = 0,
  .fb_count = 2,
  .fb_location = CAMERA_FB_IN_PSRAM,
  .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  .sccb_i2c_port = -1,
};
bool GC0308::begin() {
  M5.In_I2C.release();
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    return false;
  }
  sensor = esp_camera_sensor_get();
  return true;
}
bool GC0308::get() {
  fb = esp_camera_fb_get();
  if (!fb) {
    return false;
  }
  return true;
}
bool GC0308::free() {
  if (fb) {
    esp_camera_fb_return(fb);
    return true;
  }
  return false;
}
GC0308 Camera;

void refreshTft() {
  if (!cameraEnabled) {
    tft.pushSprite(0, 0);
  }
}

void refreshCamera() {
  camera.pushSprite(0, 0);
}

void refreshTextArea() {
  tft.fillRect(0, textTop, tft.width(), tft.height() / 2, TFT_BLACK);
  tft.setCursor(0, textTop);
  refreshTft();
}

void refreshTextAreaLarge() {
  tft.fillRect(0, textTop, tft.width(), tft.height() / 2, TFT_BLACK);
  tft.setTextFont(4);
  tft.setCursor(0, textTop);
  refreshTft();
}

void virt_analogWrite(int (&virtLED)[3], byte g) {
  uint16_t color = ((0x0 >> 3) << 11) | ((g >> 2) << 5) | (0x0 >> 3);
  tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, color);
  refreshTft();
}

void virt_BatteryWrite(int level) {
  if (level > 90) {
    tft.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_GREEN);
  } else if (level > 70) {
    tft.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_GREENYELLOW);
  } else if (level > 40) {
    tft.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_YELLOW);
  } else if (level > 20) {
    tft.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_ORANGE);
  } else {
    tft.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_RED);
  }
  refreshTft();
}

void virt_digitalWrite(int (&virtLED)[3], bool state) {
  if (state) {
    if (&virtLED == &control) {
      tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_BLUE);
    } else if (&virtLED == &connected) {
      tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_MAGENTA);
    } else if (&virtLED == &battery) {
      tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_GREENYELLOW);
    } else {
      tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_GREEN);
    }
    virtLED[2] = 1;
  } else {
    tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_BLACK);
    virtLED[2] = 0;
  }
  refreshTft();
}


void toggle_virt_led(int (&ledToToggle)[3], long toggleDelay) {
  // Toggle the state of the LED pin (write the NOT of the current state to the LED pin)
  virt_digitalWrite(ledToToggle, !(ledToToggle[2] > 0));
  delay(toggleDelay);
  virt_digitalWrite(ledToToggle, !(ledToToggle[2] > 0));
  delay(toggleDelay);
}

void toggle_led(int ledToToggle) {
  // Toggle the state of the LED pin (write the NOT of the current state to the LED pin)
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
  delay(250);
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
  delay(250);
}


void reset_rc_leds() {
  // Reset the state of the rc Virtual LEDs
  virt_digitalWrite(up, 0);
  virt_digitalWrite(down, 0);
  virt_digitalWrite(cw, 0);
  virt_digitalWrite(ccw, 0);
}

void animateControlLEDs() {
  virt_digitalWrite(connected, 1);
  delay(500);
  virt_digitalWrite(battery, 1);
  delay(500);
  virt_digitalWrite(control, 1);
  delay(500);
  virt_digitalWrite(connected, 0);
  virt_digitalWrite(battery, 0);
  virt_digitalWrite(control, 0);
}

void changeSSID(String ssid) {
  Serial.end();
  prefs_ssid = ssid;
  prefs_sn = base64::encode(serialNumberBase + ssid).substring(13);
  preferences.putString("chan_ssid", prefs_ssid);
  preferences.putString("chan_sn", prefs_sn);
  refreshTextArea();
  tft.println("Changing SSID");
  tft.println("New SSID");
  tft.println(prefs_ssid);
  tft.println("New Serial Number");
  tft.println(prefs_sn);
  tft.println("Restarting...");
  refreshTft();
  delay(2000);
  /*
  Serial.println("Changing StackChan SSID to: " + prefs_ssid);
  Serial.println("This will change the Serial number too");
  Serial.println("Changing StackChan SN to: " + prefs_sn);
  Serial.println("Restarting...");
*/
  ESP.restart();
}

void print_LED_Labels() {
  tft.setTextFont(1);
  tft.setCursor(up[0], up[1] + ledSize + 2, 1);
  tft.println("Up");
  tft.setCursor(ccw[0], ccw[1] + ledSize + 2, 1);
  tft.println("Right");
  tft.setCursor(cw[0], cw[1] + ledSize + 2, 1);
  tft.println("Left");
  tft.setCursor(down[0], down[1] + ledSize + 2, 1);
  tft.println("Down");

  tft.setCursor(connected[0], connected[1] + ledSize + 2, 1);
  tft.println("Connected");
  tft.setCursor(control[0] - 30, control[1] + ledSize + 2, 1);
  tft.println("Controlled");
  tft.setCursor(battery[0], battery[1] + ledSize + 2, 1);
  tft.println("Battery");
  refreshTft();
}


void cameraResponse(String command) {
  if (command.indexOf("cameraStop") >= 0) {
    cameraEnabled = false;
    tft.pushSprite(0, 0);
  } else if (command.indexOf("cameraStart") >= 0) {
    cameraEnabled = true;
  } else if (command.indexOf("cameraPause") >= 0) {
    tft.pushSprite(0, 0);
    delay(3000);  //show control screen for 3 seconds
  } else if (command.indexOf("cameraSnap") >= 0) {
    Camera.fb = NULL;  // empty buffer
    bool nextGet = true;
    if (Camera.get()) {
      for (int x = 0; x < 3; x++) {
        Camera.free();
        nextGet = Camera.get();  // freshen buffer
      }
      camera.pushImage(0, 0, M5.Display.width(),
                       M5.Display.height(),
                       (uint16_t*)Camera.fb->buf);
      refreshCamera();
      delay(3000);
      Camera.free();
    }
    tft.pushSprite(0, 0);
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5StackChan.begin();
  display.begin();

  tft.createSprite(display.width(), display.height());
  camera.createSprite(display.width(), display.height());

  //  tft.setRotation(2);  // Optional: Rotate the display
  tft.setFont(&fonts::Font0);
  tft.fillScreen(TFT_BLACK);
  // tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  refreshTft();


  print_LED_Labels();
  virt_digitalWrite(connected, 0);
  virt_digitalWrite(control, 0);
  reset_rc_leds();

  tft.setTextFont(2);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
    if (millis() > 5000) break;  // don't wait forever
  }
  delay(500);
  if (Serial) Serial.println("\nSerial Connected");

  WiFi.disconnect(true);
  preferences.begin("chan-control", false);
  prefs_ssid = preferences.getString("chan_ssid", "");
  prefs_sn = preferences.getString("chan_sn", "");
  refreshTextArea();
  if (prefs_ssid == "") {
    tft.println("No saved Chan SSID");
    tft.println("Use ssid command");
    tft.println("in Serial Monitor");
    tft.println("to replace SSID");
    refreshTft();
    prefs_ssid = String(ssid);
    prefs_sn = base64::encode(serialNumberBase + ssid).substring(13);
    preferences.putString("chan_ssid", prefs_ssid);
    preferences.putString("chan_sn", prefs_sn);
    delay(1000);
    tft.println("Restarting to");
    tft.println("set preferences");
    refreshTft();
    delay(2000);
    Serial.println("No value saved for StackChan SSID");
    Serial.println("Setting default SSID as: " + prefs_ssid);
    Serial.println("Setting default SN as: " + prefs_sn);
    Serial.println("Restarting...");
    ESP.restart();
  } else {
    Serial.print("Using saved StackChan SSID: ");
    Serial.println(prefs_ssid);
    Serial.print("Using saved StackChan SN: ");
    Serial.println(prefs_sn);
    tft.println("Saved Chan SSID:");
    tft.println(prefs_ssid);
    tft.println("Saved Chan SN:");
    tft.println(prefs_sn);
    refreshTft();
    delay(2000);
    serialNumber = prefs_sn;
    ssid = prefs_ssid.c_str();
  }

  //  WiFi.mode(WIFI_MODE_AP);
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  // WiFi.softAP(ssid);  // ESP-32 as access point
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(ssid) ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Serial Number: ");
  Serial.println(serialNumber);

  refreshTextAreaLarge();
  tft.println("Beginning Chan Control");
  refreshTft();
  delay(2000);
  animateControlLEDs();

  yAnglePrev = 0;
  commandMode = 0;
  batLevel = M5.Power.getBatteryLevel();
  virt_BatteryWrite(batLevel);
  Udp.begin(localPort);

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }

  Camera.sensor->set_framesize(Camera.sensor, FRAMESIZE_QVGA);
  if (Camera.get()) {
    Serial.println("Camera Get Success");
    camera.pushImage(0, 0, M5.Display.width(),
                     M5.Display.height(),
                     (uint16_t*)Camera.fb->buf);
    refreshCamera();
    delay(1000);
    Camera.free();
  }
}

void loop() {
  /* Angle unit: 10 = 1 degrees, Speed range: 0~1000 */
  /* Range X: -1280 ~ 1280 (-128° ~ 128°), Range Y: 0 ~ 900 (0° ~ 90°) */
  // M5.update();
  batLevel = M5.Power.getBatteryLevel();
  virt_BatteryWrite(batLevel);
  M5StackChan.update();
  if (cameraEnabled && Camera.get()) {
    camera.pushImage(0, 0, M5.Display.width(),
                     M5.Display.height(),
                     (uint16_t*)Camera.fb->buf);
    refreshCamera();
    Camera.free();
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command.length() == 0) command = Serial.readStringUntil('\r');
    if (command.length() > 0) {
      command.trim();
      if (command.startsWith("ssid")) {
        command.replace("ssid", "");
        command.trim();
        changeSSID(command);
      }
    }
  }

  if (!commandMode) {
    toggle_virt_led(connected, 1000);
  }

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    lastCommandTime = millis();
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    refreshTextAreaLarge();
    tft.println("Received:");
    tft.println(packetBuffer);
    if (!cameraEnabled) {
      refreshTft();
    }
    String command = String((char*)packetBuffer);
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    if (command.indexOf("command") >= 0) {
      commandMode = 1;
      virt_digitalWrite(connected, 1);
      Udp.printf("ok");
      Udp.endPacket();
    } else if (commandMode) {
      /*
      Serial.print("Received(IP/Size/Data): ");
      Serial.print(Udp.remoteIP());
      Serial.print(" / ");
      Serial.print(packetSize);
      Serial.print(" / ");
      Serial.println(packetBuffer);
*/
      if ((command.indexOf("camera") >= 0)) {  // do camera commands first
        cameraResponse(command);
      } else {
        if (command.indexOf("battery?") >= 0) {
          // Udp.printf("100\r\n");
          batLevel = M5.Power.getBatteryLevel();
          String batString = String(batLevel) + "\r\n";
          batString.toCharArray(responseBuffer, batString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("goHome") >= 0) {
          reset_rc_leds();
          M5StackChan.Motion.goHome();
          virt_digitalWrite(control, 0);
          Udp.printf("ok");
        } else if (command.indexOf("setHome") >= 0) {
          reset_rc_leds();
          M5StackChan.Motion.setCurrentPostionAsHome();
          delay(20);
          virt_digitalWrite(control, 0);
          Udp.printf("ok");
        } else if (command.indexOf("stop") >= 0) {
          M5StackChan.Motion.stop();
          reset_rc_leds();
          Udp.printf("ok");
        } else if (command.indexOf("move ") >= 0) {
          reset_rc_leds();
          int parmsPos = command.indexOf(' ');
          String moveParms = command.substring(parmsPos + 1);  // like "450 300"
          int yPos = moveParms.indexOf(' ');
          int x = moveParms.substring(0, yPos).toInt();
          int y = moveParms.substring(yPos + 1).toInt();
          if (x > 0) virt_digitalWrite(ccw, 1);
          if (x < 0) virt_digitalWrite(cw, 1);
          if (y > yAnglePrev) virt_digitalWrite(up, 1);
          if (y < yAnglePrev && y > 0) virt_digitalWrite(down, 1);
          if (y >= 0) {
            M5StackChan.Motion.move(x, y);
            yAnglePrev = y;
          } else {
            M5StackChan.Motion.move(x, 0);
            yAnglePrev = 0;
          }
          virt_digitalWrite(control, 1);
          Udp.printf("ok");
          /* Only X axis supports continuous 360° rotation. Y axis does not. */
          /* Velocity range: -1000 ~ 1000 (Negative: CW, Positive: CCW) */
        } else if (command.indexOf("rotateX") >= 0) {
          int speedPos = command.indexOf(' ') + 1;
          speed = command.substring(speedPos).toInt();
          M5StackChan.Motion.rotateX(speed);  // speed could be negative: CW if negative
          Udp.printf("ok");
        } else if (command.indexOf("moveX ") >= 0) {  // second parameter is speed
          reset_rc_leds();
          int parmsPos = command.indexOf(' ');
          String moveParms = command.substring(parmsPos + 1);  // like "450 300"
          int yPos = moveParms.indexOf(' ');
          int x = moveParms.substring(0, yPos).toInt();
          int s = moveParms.substring(yPos + 1).toInt();
          if (x > 0) virt_digitalWrite(ccw, 1);
          if (x < 0) virt_digitalWrite(cw, 1);
          M5StackChan.Motion.moveX(x, s);
          virt_digitalWrite(control, 1);
          Udp.printf("ok");
        } else if (command.indexOf("moveY ") >= 0) {  // second parameter is speed
          reset_rc_leds();
          int parmsPos = command.indexOf(' ');
          String moveParms = command.substring(parmsPos + 1);  // like "450 300"
          int yPos = moveParms.indexOf(' ');
          int y = moveParms.substring(0, yPos).toInt();
          int s = moveParms.substring(yPos + 1).toInt();
          if (y > yAnglePrev) virt_digitalWrite(up, 1);
          if (y < yAnglePrev && y > 0) virt_digitalWrite(down, 1);
          if (y >= 0) {
            M5StackChan.Motion.moveY(y, s);
            yAnglePrev = y;
          } else {
            yAnglePrev = 0;
          }
          virt_digitalWrite(control, 1);
          Udp.printf("ok");
        }
        if (!(command.indexOf("camera") >= 0)) {  //respond if not an rc command
          Udp.endPacket();
        }
      }
    }
  }
  // vTaskDelay(1);
}
