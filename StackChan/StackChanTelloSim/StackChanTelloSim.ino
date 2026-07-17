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

// TFT_eSPI tft = TFT_eSPI();  // Declare object "tft"
auto& tft = M5.Lcd;

String serialNumberBase = "CS3SN";

int up[3] = { 60, 10, 0 };
int forward[3] = { 180, 10, 0 };
int ccw[3] = { 30, 40, 0 };
int cw[3] = { 90, 40, 0 };
int left[3] = { 150, 40, 0 };
int right[3] = { 210, 40, 0 };
int down[3] = { 60, 70, 0 };
int back[3] = { 180, 70, 0 };
int connected[3] = { 10, 100, 0 };
int in_flight[3] = { 210, 100, 0 };
int battery[3] = { 110, 100, 0 };
int textTop = 150;
int ledSize = 19;

WiFiUDP Udp;  // Creation of wifi Udp instance

char packetBuffer[255];
char responseBuffer[255];

unsigned int localPort = 8889;
unsigned long motorStart = 0;
unsigned long lastCommandTime = 0;
int batLevel = 100;
int roll, pitch, throttle, yaw;
int motorOn = 0;
int SDKtimeOut = 0;
bool SDKTimerEnabled = false;
unsigned long simStartFlightTime = 0;
unsigned long simFlightTime = 0;
unsigned long simTotalFlightTime = 0;
int inFlight = 0;
int commandMode = 0;
int speed = 100;

const char *ssid = "TELLO-CSSJSO";

const char *password = "";

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

Preferences preferences;

String serialNumber = serialNumberBase + String(ssid);
String prefs_ssid;
String prefs_sn;

void refreshTextArea() {
  tft.fillRect(0, textTop, tft.width(), tft.height() / 2, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(0, textTop);
}

void refreshTextAreaLarge() {
  tft.fillRect(0, textTop, tft.width(), tft.height() / 2, TFT_BLACK);
  tft.setTextFont(4);
  tft.setCursor(0, textTop);
}

void virt_analogWrite(int (&virtLED)[3], byte g) {
  uint16_t color = ((0x0 >> 3) << 11) | ((g >> 2) << 5) | (0x0 >> 3);
  tft.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, color);
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
}

void virt_digitalWrite(int (&virtLED)[3], bool state) {
  if (state) {
    if (&virtLED == &in_flight) {
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

void flipSimulation() {
  virt_digitalWrite(up, 1);
  delay(250);
  virt_digitalWrite(down, 1);
  delay(250);
  virt_digitalWrite(ccw, 1);
  delay(250);
  virt_digitalWrite(cw, 1);
  delay(250);
  virt_digitalWrite(up, 0);
  virt_digitalWrite(down, 0);
  virt_digitalWrite(ccw, 0);
  virt_digitalWrite(cw, 0);
  virt_digitalWrite(forward, 1);
  delay(250);
  virt_digitalWrite(back, 1);
  delay(250);
  virt_digitalWrite(left, 1);
  delay(250);
  virt_digitalWrite(right, 1);
  delay(250);
  virt_digitalWrite(forward, 0);
  virt_digitalWrite(back, 0);
  virt_digitalWrite(left, 0);
  virt_digitalWrite(right, 0);
}

void reset_rc_leds() {
  // Reset the state of the rc Virtual LEDs
  virt_digitalWrite(up, 0);
  virt_digitalWrite(down, 0);
  virt_digitalWrite(cw, 0);
  virt_digitalWrite(ccw, 0);
  virt_digitalWrite(forward, 0);
  virt_digitalWrite(left, 0);
  virt_digitalWrite(right, 0);
  virt_digitalWrite(back, 0);
}

void animateSimLEDs() {
  virt_digitalWrite(connected, 1);
  virt_digitalWrite(in_flight, 1);
  virt_digitalWrite(battery, 1);
  flipSimulation();
  virt_digitalWrite(connected, 0);
  virt_digitalWrite(in_flight, 0);
  virt_digitalWrite(battery, 0);
}

void rcResponse(String command) {
  String rcParameters = "";
  reset_rc_leds();
  // extract the numbers
  int index = 0;
  int rcValues[4];
  index = command.indexOf(' ');
  rcParameters = command.substring(index + 1);
  for (int i = 0; i < 4; i++) {
    index = rcParameters.indexOf(' ');
    rcValues[i] = rcParameters.substring(0, index).toInt();
    rcParameters = rcParameters.substring(index + 1);
  }
  roll = rcValues[0];
  pitch = rcValues[1];
  throttle = rcValues[2];
  yaw = rcValues[3];
  //  Serial.println(roll);
  //  Serial.println(pitch);
  //  Serial.println(throttle);
  //  Serial.println(yaw);
  if (throttle > 0) {  //up
    virt_analogWrite(up, map(throttle, 0, 100, 0, 255));
  }
  if (pitch > 0) {  //forward
    virt_analogWrite(forward, map(pitch, 0, 100, 0, 255));
  }
  if (throttle < 0) {  //down
    virt_analogWrite(down, map(abs(throttle), 0, 100, 0, 255));
  }
  if (pitch < 0) {  //back
    virt_analogWrite(back, map(abs(pitch), 0, 100, 0, 255));
  }
  if (yaw < 0) {  //ccw
    virt_analogWrite(ccw, map(abs(yaw), 0, 100, 0, 255));
  }
  if (roll < 0) {  //left
    virt_analogWrite(left, map(abs(roll), 0, 100, 0, 255));
  }
  if (yaw > 0) {  //cw
    virt_analogWrite(cw, map(yaw, 0, 100, 0, 255));
  }
  if (roll > 0) {  //right
    virt_analogWrite(right, map(roll, 0, 100, 0, 255));
  }
  if (roll == 0 && pitch == 0 and throttle == 0 and yaw == 0) {  //hover, stop LEDs
    reset_rc_leds();
  }
}

void changeSSID(String ssid) {
  Serial.end();
  prefs_ssid = ssid;
  prefs_sn = base64::encode(serialNumberBase + ssid).substring(13);
  preferences.putString("tello_ssid", prefs_ssid);
  preferences.putString("tello_sn", prefs_sn);
  refreshTextArea();
  tft.println("Changing SSID");
  tft.println("New SSID");
  tft.println(prefs_ssid);
  tft.println("New Serial Number");
  tft.println(prefs_sn);
  tft.println("Restarting...");
  delay(2000);
  /*
  Serial.println("Changing Tello-Sim SSID to: " + prefs_ssid);
  Serial.println("This will change the Serial number too");
  Serial.println("Changing Tello-Sim SN to: " + prefs_sn);
  Serial.println("Restarting...");
*/
  ESP.restart();
}

void print_LED_Labels () {
  tft.setTextFont(1);
  tft.setCursor(up[0], up[1] + ledSize + 2);
  tft.println("Up");
  tft.setCursor(forward[0], forward[1] + ledSize + 2);
  tft.println("Forward");
  tft.setCursor(ccw[0], ccw[1] + ledSize + 2);
  tft.println("CCW");
  tft.setCursor(cw[0], cw[1] + ledSize + 2);
  tft.println("CW");
  tft.setCursor(left[0], left[1] + ledSize + 2);
  tft.println("Left");
  tft.setCursor(right[0], right[1] + ledSize + 2);
  tft.println("Right");
  tft.setCursor(down[0], down[1] + ledSize + 2);
  tft.println("Down");
  tft.setCursor(back[0], back[1] + ledSize + 2);
  tft.println("Back");

  tft.setCursor(connected[0], connected[1] + ledSize + 2);
  tft.println("Connected");
  tft.setCursor(in_flight[0] - 30, in_flight[1] + ledSize + 2);
  tft.println("In Flight");
  tft.setCursor(battery[0], battery[1] + ledSize + 2);
  tft.println("Battery");
}

void do_land() {
  toggle_virt_led(in_flight, 500);
  toggle_virt_led(in_flight, 500);
  toggle_virt_led(in_flight, 500);
  inFlight = 0;
  simFlightTime = millis() - simStartFlightTime;
  simTotalFlightTime = simTotalFlightTime + simFlightTime;
  delay(500);
  virt_digitalWrite(in_flight, 0);
}

void setup() {
  M5.begin();
//  tft.setRotation(2); // Optional: Rotate the display
  tft.setFont(&fonts::Font0);
//  tft.setFont(&FreeSans9pt7b);


  tft.fillScreen(TFT_BLACK);
  // tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  print_LED_Labels();
  virt_digitalWrite(connected, 0);
  virt_digitalWrite(in_flight, 0);
  reset_rc_leds();

  Serial.begin(115200);
  while (!Serial) {
    delay(10);
    if (millis() > 5000) break;  // don't wait forever
  }
  delay(500);
  if (Serial) Serial.println("\nSerial Connected");

  WiFi.disconnect(true);
  preferences.begin("tello-control", false);
  prefs_ssid = preferences.getString("tello_ssid", "");
  prefs_sn = preferences.getString("tello_sn", "");
  refreshTextArea();
  if (prefs_ssid == "") {
    tft.println("No saved Sim SSID");
    tft.println("Use ssid command");
    tft.println("in Serial Monitor");
    tft.println("to replace SSID");
    prefs_ssid = String(ssid);
    prefs_sn = base64::encode(serialNumberBase + ssid).substring(13);
    preferences.putString("tello_ssid", prefs_ssid);
    preferences.putString("tello_sn", prefs_sn);
    delay(1000);
    tft.println("Restarting to");
    tft.println("set preferences");
    delay(2000);
    Serial.println("No value saved for Tello Sim SSID");
    Serial.println("Setting default SSID as: " + prefs_ssid);
    Serial.println("Setting default SN as: " + prefs_sn);
    Serial.println("Restarting...");
    ESP.restart();
  } else {
    Serial.print("Using saved Tello SSID: ");
    Serial.println(prefs_ssid);
    Serial.print("Using saved Tello SN: ");
    Serial.println(prefs_sn);
    tft.println("Saved Tello SSID:");
    tft.println(prefs_ssid);
    tft.println("Saved Tello SN:");
    tft.println(prefs_sn);
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

  refreshTextAreaLarge() ;
  tft.println("Beginning Tello");
  tft.println("Simulation");
  delay(2000);
  animateSimLEDs();
  motorOn = 0;
  commandMode = 0;
  inFlight = 0;
  SDKTimerEnabled = false;
  batLevel = M5.Power.getBatteryLevel();
  virt_BatteryWrite(batLevel);
  Udp.begin(localPort);
}

void loop() {
  M5.update();
  batLevel = M5.Power.getBatteryLevel();
  virt_BatteryWrite(batLevel);  
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

  if (motorOn) {
    toggle_virt_led(in_flight, 250);
  }

  if (!commandMode) {
    toggle_virt_led(connected, 1000);
  }

  if (commandMode && SDKTimerEnabled && inFlight) {
    if ((millis() - lastCommandTime) > 21000) {
      SDKtimeOut = 0;
      do_land();
      virt_digitalWrite(in_flight, 0);
      Serial.println("SDK Timeout; no new command in over 20 seconds; Auto-Landed");
      refreshTextArea();
      tft.println("SDK TimeOut");
      tft.println("No new command");
      tft.println("In over 20 seconds");
      tft.println("Auto-Landed");
    } else {
      SDKtimeOut++;
    }
  }

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    lastCommandTime = millis();
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    refreshTextAreaLarge();
    tft.println("Received:");
    tft.println(packetBuffer);
    String command = String((char *)packetBuffer);
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    if (command.indexOf("command") >= 0) {
      commandMode = 1;
      virt_digitalWrite(connected, 1);
      Udp.printf("ok");
      Udp.endPacket();
    } else if (command.indexOf("sdktimeroff") >= 0) {
      SDKTimerEnabled = false;
      Udp.printf("ok");
      Udp.endPacket();
    } else if (command.indexOf("sdktimeron") >= 0) {
      SDKTimerEnabled = true;
      Udp.printf("ok");
      Udp.endPacket();
    } else if (commandMode) {
      Serial.print("Received(IP/Size/Data): ");
      Serial.print(Udp.remoteIP());
      Serial.print(" / ");
      Serial.print(packetSize);
      Serial.print(" / ");
      Serial.println(packetBuffer);
      if ((command.indexOf("rc") >= 0) && inFlight) {  // do rc command first
        rcResponse(command);
      } else {
        if (command.indexOf("battery?") >= 0) {
          // Udp.printf("100\r\n");
          batLevel = M5.Power.getBatteryLevel();
          String batString = String(batLevel) + "\r\n";
          batString.toCharArray(responseBuffer, batString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("reboot") >= 0) {
          ESP.restart();
        } else if (command.indexOf("time?") >= 0) {
          String timeString = String(simTotalFlightTime / 1000) + "s\r\n";
          timeString.toCharArray(responseBuffer, timeString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("speed?") >= 0) {
          String speedString = String(speed) + "\r\n";
          speedString.toCharArray(responseBuffer, speedString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("speed") >= 0) {
          int speedPos = command.indexOf(' ') + 1;
          speed = command.substring(speedPos).toInt();
          Udp.printf("ok");
        } else if (command.indexOf("ssid?") >= 0) {
          Udp.printf(ssid);
        } else if (command.indexOf("wifi?") >= 0) {
          String snrString = String(random(50, 90)) + "\r\n";
          snrString.toCharArray(responseBuffer, snrString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("sdk?") >= 0) {
          String sdkString = "30";
          sdkString.toCharArray(responseBuffer, sdkString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("sn?") >= 0) {
          serialNumber.toCharArray(responseBuffer, serialNumber.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("temp?") >= 0) {
          String tempString = String((int)(temperatureRead() + 0.5)) + "C\r\n";
          tempString.toCharArray(responseBuffer, tempString.length() + 1);
          Udp.printf(responseBuffer);
        } else if (command.indexOf("motoron") >= 0) {
          motorStart = millis();  // mark time of motor on
          motorOn = 1;
          delay(1000);
          virt_digitalWrite(in_flight, 1);
          Udp.printf("ok");
        } else if (command.indexOf("motoroff") >= 0) {
          motorOn = 0;
          delay(1000);
          virt_digitalWrite(in_flight, 0);
          Udp.printf("ok");
        } else if ((command.indexOf("takeoff") >= 0) && !inFlight) {
          motorStart = millis();  // mark time of motor start
          toggle_virt_led(in_flight, 500);
          toggle_virt_led(in_flight, 500);
          toggle_virt_led(in_flight, 500);
          inFlight = 1;
          simStartFlightTime = millis();
          delay(500);
          virt_digitalWrite(in_flight, 1);
          Udp.printf("ok");
        } else if (inFlight) {
          reset_rc_leds();
          if (command.indexOf("land") >= 0) {
            do_land();
            Udp.printf("ok");
          } else if (command.indexOf("emergency") >= 0) {  // stop all LEDs
            toggle_virt_led(connected, 500);
            virt_digitalWrite(connected, 1);
            virt_digitalWrite(in_flight, 0);
            delay(5000);
            virt_digitalWrite(connected, 0);
            inFlight = 0;
            commandMode = 0;
            Udp.printf("ok");
          } else if (command.indexOf("stop") >= 0) {
            Udp.printf("ok");
          } else if (command.indexOf("ccw") >= 0) {  // catch ccw first because cw satisfies ccw
            virt_digitalWrite(ccw, 1);
            delay(3000);
            virt_digitalWrite(ccw, 0);
            Udp.printf("ok");
          } else if (command.indexOf("cw") >= 0) {
            virt_digitalWrite(cw, 1);
            delay(3000);
            virt_digitalWrite(cw, 0);
            Udp.printf("ok");
          } else if (command.indexOf("up") >= 0) {
            virt_digitalWrite(up, 1);
            delay(3000);
            virt_digitalWrite(up, 0);
            Udp.printf("ok");
          } else if (command.indexOf("down") >= 0) {
            virt_digitalWrite(down, 1);
            delay(3000);
            virt_digitalWrite(down, 0);
            Udp.printf("ok");
          } else if (command.indexOf("right") >= 0) {
            virt_digitalWrite(right, 1);
            delay(3000);
            virt_digitalWrite(right, 0);
            Udp.printf("ok");
          } else if (command.indexOf("left") >= 0) {
            virt_digitalWrite(left, 1);
            delay(3000);
            virt_digitalWrite(left, 0);
            Udp.printf("ok");
          } else if (command.indexOf("forward") >= 0) {
            virt_digitalWrite(forward, 1);
            delay(3000);
            virt_digitalWrite(forward, 0);
            Udp.printf("ok");
          } else if (command.indexOf("back") >= 0) {
            virt_digitalWrite(back, 1);
            delay(3000);
            virt_digitalWrite(back, 0);
            Udp.printf("ok");
          } else if (command.indexOf("flip") >= 0) {
            flipSimulation();
            delay(1000);
            Udp.printf("ok");
          } else if (command.indexOf("go ") >= 0) {
            int parmsPos = command.indexOf(' ');
            String goParms = command.substring(parmsPos + 1);  // like "-40 40"
            int yPos = goParms.indexOf(' ');
            int x = goParms.substring(0, yPos).toInt();
            int y = goParms.substring(yPos + 1).toInt();
            if ((x > 0) && (y == 0)) {
              virt_digitalWrite(forward, 1);
              delay(3000);
              virt_digitalWrite(forward, 0);
            } else if ((x < 0) && (y == 0)) {
              virt_digitalWrite(back, 1);
              delay(3000);
              virt_digitalWrite(back, 0);
            } else if ((x == 0) && (y < 0)) {
              virt_digitalWrite(right, 1);
              delay(3000);
              virt_digitalWrite(right, 0);
            } else if ((x == 0) && (y > 0)) {
              virt_digitalWrite(left, 1);
              delay(3000);
              virt_digitalWrite(left, 0);
            } else if ((x > 0) && (y < 0)) {
              virt_digitalWrite(forward, 1);
              virt_digitalWrite(right, 1);
              delay(3000);
              virt_digitalWrite(forward, 0);
              virt_digitalWrite(right, 0);
            } else if ((x < 0) && (y < 0)) {
              virt_digitalWrite(back, 1);
              virt_digitalWrite(right, 1);
              delay(3000);
              virt_digitalWrite(back, 0);
              virt_digitalWrite(right, 0);
            } else if ((x > 0) && (y > 0)) {
              virt_digitalWrite(forward, 1);
              virt_digitalWrite(left, 1);
              delay(3000);
              virt_digitalWrite(forward, 0);
              virt_digitalWrite(left, 0);
            } else if ((x < 0) && (y > 0)) {
              virt_digitalWrite(back, 1);
              virt_digitalWrite(left, 1);
              delay(3000);
              virt_digitalWrite(back, 0);
              virt_digitalWrite(left, 0);
            }
            Udp.printf("ok");
          }
        } else {
          Udp.printf("error");
        }
        if (!(command.indexOf("rc ") >= 0)) {  //respond if not an rc command
          Udp.endPacket();
        }
      }
    }
    // vTaskDelay(1);
  }
}