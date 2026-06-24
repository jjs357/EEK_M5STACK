#include "DroneCommon.h"

void clearText() {
  display.fillRect(0, 0, display.width(), display.height() / 2, TFT_BLACK);
  display.setFont(&fonts::Font2);
  display.setCursor(0, 0);
}

void clearTextLarge() {
  display.fillRect(0, 0, display.width(), display.height() / 2, TFT_BLACK);
  display.setFont(&fonts::FreeSans9pt7b);
  display.setCursor(0, 0);
}

void connectToWiFi(const char *ssid) {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  String ssidString = String((char *)ssid);
  clearText();
  display.println("Connecting to:");
  display.println(ssidString);
  // delete old config
  WiFi.disconnect(true);
  disconnected_tick = 0;
  //Initiate connection
  //WiFi.begin(ssid, pwd);
  WiFi.begin(ssid);
  Serial.println("Waiting for WIFI connection...");
  int reconnectTick = 0;
  while (!lastCommandOK && reconnectTick < 10) {
    Serial.print(".");
    display.print(".");
    reconnectTick++;
    delay(1000);
  }
  if (reconnectTick >= 10) {  // waited 10 seconds
    Serial.println("NOT CONNECTED");
    Serial.println("Use connect Command in Serial Monitor Input with active SSID");
    clearText();
    display.println("NOT CONNECTED");
    display.println("Use connect command");
    display.println("in Serial Monitor");
    display.println("With active SSID");
    delay(2000);
  }
}

void virt_BatteryWrite(int level) {
  if (level > 90) {
    display.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_GREEN);
  } else if (level > 70) {
    display.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_GREENYELLOW);
  } else if (level > 40) {
    display.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_YELLOW);
  } else if (level > 20) {
    display.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_ORANGE);
  } else {
    display.fillRect(battery[0], battery[1], ledSize, ledSize, TFT_RED);
  }
}

void virt_digitalWrite(int (&virtLED)[3], bool state) {
  if (state) {
    if (&virtLED == &flight) {
      display.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_BLUE);
    } else if (&virtLED == &conn) {
      display.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_MAGENTA);
    } else if (&virtLED == &battery) {
      display.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_GREENYELLOW);
    } else {
      display.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_GREEN);
    }
    virtLED[2] = 1;
  } else {
    display.fillRect(virtLED[0], virtLED[1], ledSize, ledSize, TFT_BLACK);
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

void animateVirtLEDs() {
  virt_digitalWrite(conn, 1);
  delay(500);
  virt_digitalWrite(battery, 1);
  delay(500);
  virt_digitalWrite(flight, 1);
  delay(500);
  virt_digitalWrite(use_imu, 1);
  delay(500);
  virt_digitalWrite(conn, 0);
  virt_digitalWrite(battery, 0);
  virt_digitalWrite(flight, 0);
  virt_digitalWrite(use_imu, 0);
}

void toggle_led(int ledToToggle) {
  // Toggle the state of the LED pin (write the NOT of the current state to the LED pin)
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
  delay(250);
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
  delay(250);
}

void print_LED_Labels() {
  display.setFont(&fonts::Font0);
  display.setCursor(conn[0] - 20, conn[1] + ledSize + 2, 1);
  display.println("Connected");
  display.setCursor(flight[0] - 20, flight[1] + ledSize + 2, 1);
  display.println("In Flight");
  display.setCursor(battery[0] - 20, battery[1] + ledSize + 2, 1);
  display.println("Battery");
  display.setCursor(use_imu[0] - 20, use_imu[1] + ledSize + 2);
  display.println("Using IMU");
}

void imuValuesUpdate() {
  imu.getGyroData(&gyroX, &gyroY, &gyroZ);
  imu.getAccelData(&accX, &accY, &accZ);
}

float imuGetAngleX() {
  return atan2(accY, sqrt(accX * accX + accZ * accZ)) * 180 / PI;
}

float imuGetAngleY() {
  return atan2(accX, sqrt(accY * accY + accZ * accZ)) * 180 / PI;
}

void run_command(String command, int udp_delay_ticks, int waitAfterDelay) {
  int packetSize = 0;
  last_command_millis = millis();
  boolean responseExpected = true;
  lastCommandOK = true;
  clearTextLarge();
  digitalWrite(COMMAND_TICK, HIGH);  // off when high
  display.println("Command:");
  display.println(command);
  // Special delay cases
  if (command.indexOf("takeoff") >= 0) udp_delay_ticks = 40;
  if (command.indexOf("land") >= 0) udp_delay_ticks = 20;
  if (command.indexOf("rc ") >= 0) {
    udp_delay_ticks = 0;
    lastCommandOK = true;  // rc commands are always OK
    responseExpected = false;
    digitalWrite(COMMAND_TICK, LOW);  // on when LOW
  } else {
    Serial.println(command);
  }
  memset(buffer, 0, 50);
  command.getBytes(buffer, command.length() + 1);
  //only send data when connected
  //Send a packet
  udp.beginPacket(udpAddress, udpPort);
  udp.write(buffer, command.length() + 1);
  udp.endPacket();
  // Serial.println("endPacket called");
  // allow for an rc command to not need any further processing after sending.
  memset(buffer, 0, 50);
  for (int x = 0; x < udp_delay_ticks; x++) {
    delay(500);
    toggle_led(COMMAND_TICK);
    // Serial.print(x);
    packetSize = udp.parsePacket();
    if (packetSize) break;
  }
  // Serial.println("packetSize: " + String(packetSize));
  if (packetSize && responseExpected) {
    if (udp.read(buffer, 50) > 0) {
      digitalWrite(COMMAND_TICK, HIGH);  // off when high
      String commandResponse = String((char *)buffer);
      Serial.println(commandResponse);
      display.println("Response:");
      display.println(commandResponse);
      bool parseResponse = (commandResponse.indexOf("forced stop") == -1)
                           && (commandResponse.indexOf("error") == -1)
                           && (commandResponse.indexOf("ok") == -1);
      if (command.equalsIgnoreCase("battery?") && parseResponse) {
        // Ignoring Battery Level for now
        int battery = commandResponse.toInt();
        virt_BatteryWrite(battery);
        lastCommandOK = true;
        digitalWrite(COMMAND_TICK, HIGH);  // LED off when HIGH
      } else if (commandResponse.indexOf("ok") >= 0) {
        lastCommandOK = true;
        digitalWrite(COMMAND_TICK, HIGH);  // LED off when HIGH
      } else if (commandResponse.indexOf("timeout") >= 0) {
        lastCommandOK = true;
        Serial.println("Command timed out, ignoring for now");
      } else if (commandResponse.indexOf("forced stop") >= 0) {
        Serial.println("Unexpected forced stop Response; treat as error");
        lastCommandOK = false;
      } else if (commandResponse.indexOf("error") >= 0) {
        Serial.println("Unexpected error response; treat as error");
        lastCommandOK = false;
      }
    } else if (in_flight) {
      clearTextLarge();
      display.println("No command response: ");
      display.println("Landing NOW!");
      lastCommandOK = false;
      digitalWrite(COMMAND_TICK, HIGH);  // led off when HIGH
    }
  } else if (command.indexOf("command") >= 0) {  // no response
    clearTextLarge();
    display.println("No command response: ");
    display.println("Restarting");
    lastCommandOK = false;
    digitalWrite(COMMAND_TICK, HIGH);  // led off when HIGH
  }
  delay(waitAfterDelay);
}

void Run_Demo_FlightPlan() {
  Serial.println("Run_Demo_FlightPlan");
  run_command("motoron", 10, 5000);
  run_command("motoroff", 10, 1000);
  run_command("battery?", 10, 0);
  run_command("takeoff", 40, 0);
  if (lastCommandOK) virt_digitalWrite(flight, 1);
  run_command("up 50", 20, 2000);
  run_command("cw 90", 20, 2000);
  run_command("ccw 90", 20, 2000);
  run_command("down 50", 20, 2000);
  run_command("land", 40, 0);
  if (lastCommandOK) virt_digitalWrite(flight, 0);
  run_command("battery?", 10, 0);
}

void run_flight_plan() {
  if (connected) {
    Serial.println("run_flight_plan");
    run_command("motoron", 10, 5000);
    run_command("motoroff", 10, 1000);
    run_command("takeoff", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 1);
    run_command("up 70", 20, 2000);
    run_command("cw 90", 20, 2000);
    run_command("ccw 90", 20, 2000);
    run_command("down 70", 20, 2000);
    run_command("land", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 0);
  }
}

void run_flight_plan_A() {
  if (connected) {
    Serial.println("run_flight_plan_A");
    run_command("motoron", 10, 5000);
    run_command("motoroff", 10, 1000);
    run_command("takeoff", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 1);
    run_command("up 70", 20, 2000);
    run_command("forward 70", 20, 2000);
    run_command("left 70", 20, 2000);
    run_command("back 70", 20, 2000);
    run_command("right 70", 20, 2000);
    run_command("land", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 0);
  }
}

void run_flight_plan_B() {
  if (connected) {
    Serial.println("run_flight_plan_B");
    run_command("motoron", 10, 5000);
    run_command("motoroff", 10, 1000);
    run_command("takeoff", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 1);
    // to-do-1: Add several more commands here
    run_command("cw 180", 20, 2000);
    run_command("ccw 180", 20, 2000);
    run_command("land", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 0);
  }
}

void run_CCW_circle_flight() {
  run_command("rc 0 0 0 0", 0, 1000);
  run_command("rc 0 50 0 -100", 0, 4000);  // fly a CCW circular arc
  run_command("rc 0 0 0 0", 0, 1000);
}

void run_CW_circle_flight() {
  run_command("rc 0 0 0 0", 0, 1000);
  run_command("rc 0 50 0 100", 0, 4000);  // fly a CW circular arc
  run_command("rc 0 0 0 0", 0, 1000);
}


void onTakeoffButtonPressed() {
  if (in_flight) {
    run_command("land", 20, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 0);
    in_flight = false;
  } else {
    run_command("takeoff", 40, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 1);
    in_flight = true;
  }
}

void onKillButtonPressed() {
  Serial.println("KILL button is pressed");
  if (in_flight) {
    run_command("emergency", 10, 0);
    if (lastCommandOK) virt_digitalWrite(flight, 0);
    in_flight = false;
    clearTextLarge();
    display.println("Emergency");
    display.println("Restarting!");
    Serial.println("Restarting: Emergency command ");
    delay(3000);
    ESP.restart();
  } else {
    run_flight_plan();
  }
}

void onPowerButtonPressed() {
  if (use_IMUtoFly) {
    use_IMUtoFly = false;
    virt_digitalWrite(use_imu, false);
  }
  Serial.println("Power button is pressed");
  if (in_flight) {
    run_CW_circle_flight();
  } else {
    run_flight_plan_A();
  }
}

void onBtnBPressed() {
  // Serial.println("Right side button pressed");
  if (!in_flight) {
    clearTextLarge();
    if (use_IMUtoFly) {
      use_IMUtoFly = false;
      display.println("IMU Flying");
      display.println("Disabled");
    } else {
      use_IMUtoFly = true;
      display.println("IMU Flying");
      display.println("Enabled");
    }
    virt_digitalWrite(use_imu, use_IMUtoFly);
  } else {
    run_CCW_circle_flight();
  }
}

void check_serial() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    char SSID[65];
    if (command.length() == 0) command = Serial.readStringUntil('\r');
    if (command.length() > 0) {
      command.trim();
      if (command.startsWith("connect")) {
        command.replace("connect", "");
        command.trim();
        strcpy(SSID, command.c_str());
        preferences.putString("tello_ssid", command);
        connectToWiFi(command.c_str());
      } else if (connected) {
        if (command.indexOf("takeoff") >= 0) {
          run_command(command, 40, 0);
          if (lastCommandOK) virt_digitalWrite(flight, 1);
          in_flight = true;
        } else if (command.indexOf("land") >= 0) {
          run_command(command, 40, 0);
          if (lastCommandOK) virt_digitalWrite(flight, 0);
          in_flight = false;
        } else {
          run_command(command, 40, 0);
        }
      }
    }
  }
}
