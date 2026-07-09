#include "ControlCommon.h"

void clearText() {
  display.clearDisplay();
  display.setCursor(0, 0);
}

void toggle_led(int ledToToggle) {
  // Toggle the state of the LED pin (write the NOT of the current state to the LED pin)
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
  delay(250);
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
  delay(250);
}

void connectToWiFi(const char *ssid) {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  String ssidString = String((char *)ssid);
  clearText();
  display.println("Connecting to:");
  display.println(ssidString);
  display.display();

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
    display.display();
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
    display.display();
    delay(2000);
  }
}

void animateEEKLEDs() {
  digitalWrite(LED_USING_MPU, HIGH);
  delay(250);
  digitalWrite(IN_CONTROL, HIGH);
  delay(250);
  digitalWrite(LED_CONN_GREEN, HIGH);
  delay(250);
  digitalWrite(LED_BATT_GREEN, HIGH);
  delay(250);
  digitalWrite(LED_BATT_YELLOW, HIGH);
  delay(250);
  digitalWrite(LED_BATT_RED, HIGH);
  delay(250);
  digitalWrite(LED_USING_MPU, LOW);
  digitalWrite(IN_CONTROL, LOW);
  digitalWrite(LED_CONN_GREEN, LOW);
  digitalWrite(LED_BATT_GREEN, LOW);
  digitalWrite(LED_BATT_YELLOW, LOW);
  digitalWrite(LED_BATT_RED, LOW);
}

void run_command(String command, int udp_delay_ticks, int waitAfterDelay) {
  int packetSize = 0;
  last_command_millis = millis();
  boolean responseExpected = true;
  lastCommandOK = true;
  clearText();
  digitalWrite(COMMAND_TICK, LOW);
  display.println("Command:");
  display.println(command);
  display.display();
  Serial.println(command);
  // Special delay cases
  if ((command.indexOf("battery") >= 0) || (command.indexOf("command") >= 0)) {
    udp_delay_ticks = 20;
    responseExpected = true;
    digitalWrite(COMMAND_TICK, LOW);
  }

  if (command.indexOf("camera") >= 0) {
    udp_delay_ticks = 0;  // no response for camera commands
    lastCommandOK = true;
    cameraOn = (command.indexOf("Start") >= 0) ? true : false;
    responseExpected = false;
    digitalWrite(COMMAND_TICK, LOW);
  }

  if (command.indexOf("rc") >= 0) {  // Actuator commands
    udp_delay_ticks = 0;
    lastCommandOK = true;  // rc commands are always OK
    responseExpected = false;
    digitalWrite(COMMAND_TICK, LOW);
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
      digitalWrite(COMMAND_TICK, LOW);
      String commandResponse = String((char *)buffer);
      Serial.println(commandResponse);
      display.println("Response:");
      display.println(commandResponse);
      display.display();
      if (command.equalsIgnoreCase("battery?")) {
        int battery = commandResponse.toInt();
        if (battery > 90) {
          digitalWrite(LED_BATT_GREEN, HIGH);
          digitalWrite(LED_BATT_RED, LOW);
          digitalWrite(LED_BATT_YELLOW, LOW);
        } else if (battery > 70) {
          digitalWrite(LED_BATT_GREEN, HIGH);
          digitalWrite(LED_BATT_RED, LOW);
          digitalWrite(LED_BATT_YELLOW, HIGH);
        } else if (battery > 40) {
          digitalWrite(LED_BATT_GREEN, LOW);
          digitalWrite(LED_BATT_RED, LOW);
          digitalWrite(LED_BATT_YELLOW, HIGH);
        } else if (battery > 20) {
          digitalWrite(LED_BATT_GREEN, LOW);
          digitalWrite(LED_BATT_RED, HIGH);
          digitalWrite(LED_BATT_YELLOW, HIGH);
        } else {
          digitalWrite(LED_BATT_GREEN, LOW);
          digitalWrite(LED_BATT_RED, HIGH);
          digitalWrite(LED_BATT_YELLOW, LOW);
        }
        lastCommandOK = true;
        digitalWrite(COMMAND_TICK, LOW);
      } else if (command.equalsIgnoreCase("command")) {
        lastCommandOK = true;
        digitalWrite(COMMAND_TICK, LOW);
      } else if (command.equalsIgnoreCase("goHome")) {
        lastCommandOK = true;
        digitalWrite(IN_CONTROL, LOW);  // At Home
        in_control = 0;
        digitalWrite(COMMAND_TICK, LOW);
      } else if (commandResponse.indexOf("ok") >= 0) {
        lastCommandOK = true;
        // digitalWrite(IN_CONTROL, HIGH);     // still in control
        digitalWrite(COMMAND_TICK, LOW);
      }
    }
  } else if (command.indexOf("command") >= 0) {  // no response
    clearText();
    display.println("No command response: ");
    display.println("Restarting");
    display.display();
    lastCommandOK = false;
    digitalWrite(COMMAND_TICK, LOW);
  }
  delay(waitAfterDelay);
}

void run_demo_controlPlan() {
  toggle_led(IN_CONTROL);
  Serial.println("run_demo_controlPlan");
  run_command("move 0 0", 20, 0);
  run_command("move 0 450", 20, 500);
  run_command("cameraStart", 0, 3000);
  run_command("move 0 300", 20, 3000);
  run_command("cameraStop", 0, 500);
  run_command("move -900 -450", 20, 0);
  delay(1000);
  run_command("move 900 450", 20, 0);
  delay(1000);
  run_command("move 900 -450", 20, 0);
  delay(1000);
  run_command("goHome", 20, 0);
  toggle_led(IN_CONTROL);
}

void run_rotation_plan() {
  Serial.println("run_rotation_plan");
  toggle_led(IN_CONTROL);
  run_command("move 0 0", 20, 0);
  run_command("rotateX 500", 20, 0);
  delay(2000);
  run_command("stop", 20, 0);
  run_command("rotateX -500", 20, 0);
  delay(2000);
  run_command("stop", 20, 0);
  run_command("move 0 0", 20, 0);
  toggle_led(IN_CONTROL);
}

void onTakeoffButtonPressed(Button2 &btn) {
  if (in_control) {
    run_command("goHome", 20, 0);
    digitalWrite(IN_CONTROL, LOW);
    in_control = false;
    Xangle = 0;
    Yangle = 0;
    rcTime = 0;
    run_command("battery?", 20, 2000);
  } else {
    run_command("move 0 0", 40, 100);
    run_command("move 0 300", 40, 100);
    run_command("moveX 300 500", 40, 100);
    run_command("moveX -300 500", 40, 100);
    run_command("moveX 0 500", 40, 100);
    run_command("move 0 0", 40, 0);
    digitalWrite(IN_CONTROL, HIGH);
    in_control = true;
    rcTime = millis();
    run_command("battery?", 20, 2000);
  }
}

void onUpButtonTapped(Button2 &btn) {
  if (in_control) {
    Yangle = min(Yangle + 200, 900);
    run_command("moveY " + String(Yangle) + " 500", 20, 0);
  } else {
    // Serial.println("Up button is pressed when not under control");
    clearText();
    display.println("Up Button Pressed");
    display.println("Running Control Plan");
    display.display();
    delay(1000);
    run_demo_controlPlan();
  }
}

void onDownButtonTapped(Button2 &btn) {
  if (in_control) {
    Yangle = max(Yangle - 200, 0);
    run_command("moveY " + String(Yangle) + " 500", 20, 0);
  } else {
    Serial.println("Down button is pressed when not under control");
    clearText();
    display.println("Down Button Pressed");
    display.println("Changing MPU Status");
    if (use_MPUtoControl) {
      display.println("MPU Control Disabled");
    } else {
      display.println("MPU Control Enabled");
    }
    display.display();
    use_MPUtoControl = !use_MPUtoControl;
    digitalWrite(LED_USING_MPU, use_MPUtoControl);
    delay(1000);
  }
}

void onCWButtonPressed(Button2 &btn) {
  if (in_control) {
    Xangle = max(Xangle - 300, -1280);
    run_command("moveX " + String(Xangle) + " 500", 20, 0);
  } else {
    // Serial.println("CW button is pressed when not under control");
    clearText();
    display.println("CW Button Pressed");
    display.println("Changing");
    display.println("StackChan");
    display.println("Home");
    display.display();
    delay(1000);
    Serial.println("set_home_manually");
    clearText();
    display.println("Move Stack");
    display.println("to New Home");
    display.println("Press CW");
    display.println("to Continue");
    display.display();
    while (true) {
      if (digitalRead(CW_PIN) == HIGH) {
        clearText();
        run_command("setHome", 20, 1000);
        run_command("goHome", 20, 0);
        break;
      }
      delay(50);
    }
  }
}

void onCCWButtonPressed(Button2 &btn) {
  if (in_control) {
    Xangle = min(Xangle + 300, +1280);
    run_command("moveX " + String(Xangle) + " 500", 20, 0);
  } else {
    // Serial.println("CCW button is pressed when not under control");
    clearText();
    display.println("CCW Button Pressed");
    display.println("Running Rotation Action");
    display.display();
    delay(1000);
    toggle_led(IN_CONTROL);
    rotateXdirCW = !rotateXdirCW;
    (rotateXdirCW) ? run_command("rotateX -500", 20, 0) : run_command("rotateX 500", 20, 0);
    in_rotation = true;
    while (true) {
      if (digitalRead(CCW_PIN) == HIGH) {
        clearText();
        run_command("stop", 20, 0);
        in_rotation = false;
        break;
      }
      delay(50);
    }
  }
}

void onKillButtonPressed(Button2 &btn) {
  Serial.println("KILL button is pressed");
  if (in_control) {
    run_command("goHome", 20, 0);
    if (lastCommandOK) digitalWrite(IN_CONTROL, LOW);
    in_control = false;
    clearText();
    display.println("Chan returned");
    display.println("Home");
    display.setCursor(0, 40);
    display.println("Restarting!");
    Serial.println("Restarting: Emergency command ");
    delay(3000);
    ESP.restart();
  } else {
    run_command("goHome", 20, 0);
    // toggle camera state
    if (cameraOn) {
      run_command("cameraStop", 0, 100);
    } else {
      run_command("cameraStart", 0, 100);
    }
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
          onTakeoffButtonPressed(takeoffButton);
        } else if (command.indexOf("land") >= 0) {
          onTakeoffButtonPressed(takeoffButton);
        } else if (command.indexOf("emergency") >= 0) {
          onKillButtonPressed(killButton);
        } else {
          run_command(command, 20, 0);
        }
      }
    }
  }
}
