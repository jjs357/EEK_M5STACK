#include "ControlCommon.h"

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
    if (&virtLED == &control) {
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
  virt_digitalWrite(control, 1);
  delay(500);
  virt_digitalWrite(use_imu, 1);
  delay(500);
  virt_digitalWrite(conn, 0);
  virt_digitalWrite(battery, 0);
  virt_digitalWrite(control, 0);
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
  display.setCursor(conn[0] - 20, conn[1] + ledSize + 2);
  display.println("Connected");
  display.setCursor(control[0] - 20, control[1] + ledSize + 2);
  display.println("In Control");
  display.setCursor(use_imu[0] - 20, use_imu[1] + ledSize + 2);
  display.println("Using IMU");
  display.setCursor(battery[0] - 20, battery[1] + ledSize + 2);
  display.println("Battery");
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
  Serial.println(command);
  // Special delay cases
  if ((command.indexOf("battery") >= 0) || (command.indexOf("command") >= 0)) {
    udp_delay_ticks = 20;
    responseExpected = true;
    digitalWrite(COMMAND_TICK, LOW);  // on when LOW
  }
  if (command.indexOf("rc") >= 0) {  // Actuator commands
    udp_delay_ticks = 0;
    lastCommandOK = true;  // rc commands are always OK
    responseExpected = false;
    digitalWrite(COMMAND_TICK, LOW);  // on when LOW
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
      if (command.equalsIgnoreCase("battery?")) {
        int battery = commandResponse.toInt();
        virt_BatteryWrite(battery);
        lastCommandOK = true;
        digitalWrite(COMMAND_TICK, HIGH);  // LED off when HIGH
      } else if (command.equalsIgnoreCase("command")) {
        lastCommandOK = true;
        digitalWrite(COMMAND_TICK, HIGH);  // LED off when HIGH
      } else if (command.equalsIgnoreCase("goHome")) {
        lastCommandOK = true;
        virt_digitalWrite(control, 0);  // At Home
        in_control = 0;
        digitalWrite(COMMAND_TICK, HIGH);  // LED off when HIGH
      } else if (commandResponse.indexOf("ok") >= 0) {
        lastCommandOK = true;
        // virt_digitalWrite(control, 1);     // still in control
        digitalWrite(COMMAND_TICK, HIGH);  // LED off when HIGH
      }
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

void set_home_manually() {
  if (connected) {
    bool buttonWasPressed = false;
    int waitCounter = 0;
    Serial.println("set_home_manually");
    clearTextLarge();
    display.println("Move Stack");
    display.println("to New Home");
    display.println("Press A");
    display.println("to Continue");
    M5.update();
    while (true) {
      if (M5.BtnA.wasPressed()) {
        clearTextLarge();
        run_command("setHome", 20, 1000);
        run_command("goHome", 20, 0);
        break;
      }
      delay(50);
      M5.update();
    }
  }
}

void run_demo_controlPlan() {
  Serial.println("run_demo_controlPlan");
  toggle_virt_led(control, 500);
  run_command("move 0 0", 20, 0);
  run_command("move 0 450", 20, 0);
  run_command("snap", 20, 0);
  delay(1000);
  run_command("move 0 300", 20, 0);
  run_command("snap", 20, 0);
  delay(1000);
  run_command("move -900 -450", 20, 0);
  delay(1000);
  run_command("move 900 450", 20, 0);
  delay(1000);
  run_command("move 900 -450", 20, 0);
  delay(1000);
  run_command("goHome", 20, 0);
  toggle_virt_led(control, 500);
}

void run_flight_plan_A() {
  Serial.println("run_flight_plan_A");
  toggle_virt_led(control, 500);
  run_command("move 0 0", 40, 1000);
  run_command("rotateX 500", 40, 0);
  delay(2000);
  run_command("stop", 40, 500);
  run_command("rotateX -500", 40, 0);
  delay(2000);
  run_command("stop", 40, 500);
  run_command("move 0 0", 40, 0);
  toggle_virt_led(control, 500);
}

void onTakeoffButtonPressed() {
  if (in_control) {
    run_command("goHome", 20, 0);
    virt_digitalWrite(control, 0);
    in_control = false;
    run_command("battery?", 20, 2000);
  } else {
    run_command("move 0 0", 40, 100);
    run_command("moveY 300 500", 40, 100);
    run_command("moveX 300 500", 40, 100);
    run_command("moveX -300 500", 40, 100);
    run_command("moveX 0 500", 40, 100);
    run_command("move 0 0", 40, 0);
    virt_digitalWrite(control, 1);
    in_control = true;
    use_IMUtoControl = !use_IMUtoControl;  // toggle IMU control setting
    virt_digitalWrite(use_imu, use_IMUtoControl);
    run_command("battery?", 20, 2000);
  }
}

void onRotateButtonPressed() {
  if (use_IMUtoControl) {
    use_IMUtoControl = false;
    virt_digitalWrite(use_imu, false);
  }
  if (in_rotation) {
    run_command("stop", 20, 0);
    in_rotation = false;
  } else if (in_control) {
    rotateXdirCW = !rotateXdirCW;
    (rotateXdirCW) ? run_command("rotateX -500", 20, 0) : run_command("rotateX 500", 20, 0);
    in_rotation = true;
  }
}

void onPowerButtonPressed() {
  if (use_IMUtoControl) {
    use_IMUtoControl = false;
    virt_digitalWrite(use_imu, false);
  }
  Serial.println("Power button is pressed");
  if (in_control) {
    run_flight_plan_A();
  } else {
    run_demo_controlPlan();
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
        preferences.putString("chan_ssid", command);
        connectToWiFi(command.c_str());
      } else if (connected) {
        if (command.indexOf("takeoff") >= 0) {
          onTakeoffButtonPressed();
        } else if (command.indexOf("land") >= 0) {
          onTakeoffButtonPressed();
        } else {
          run_command(command, 20, 0);
        }
      }
    }
  }
}
