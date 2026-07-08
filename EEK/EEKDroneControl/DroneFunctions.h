#include "DroneCommon.h"

void toggle_led(int ledToToggle) {
  // Toggle the state of the LED pin (write the NOT of the current state to the LED pin)
  digitalWrite(ledToToggle, !digitalRead(ledToToggle));
}

void animateEEKLEDs() {
  digitalWrite(LED_CONN_RED, HIGH);
  delay(250);
  digitalWrite(IN_FLIGHT, HIGH);
  delay(250);
  digitalWrite(LED_CONN_GREEN, HIGH);
  delay(250);
  digitalWrite(LED_BATT_GREEN, HIGH);
  delay(250);
  digitalWrite(LED_BATT_YELLOW, HIGH);
  delay(250);
  digitalWrite(LED_BATT_RED, HIGH);
  delay(250);
  digitalWrite(LED_CONN_RED, LOW);
  digitalWrite(IN_FLIGHT, LOW);
  digitalWrite(LED_CONN_GREEN, LOW);
  digitalWrite(LED_BATT_GREEN, LOW);
  digitalWrite(LED_BATT_YELLOW, LOW);
  digitalWrite(LED_BATT_RED, LOW);
}

void connectToWiFi(const char *ssid) {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  String ssidString = String((char *)ssid);
  display.clearDisplay();
  display.setCursor(0, 0);
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
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("NOT CONNECTED");
    display.println("Use connect command");
    display.println("in Serial Monitor");
    display.println("With active SSID");
    display.display();
    delay(2000);
  }
}

void run_command(String command, int udp_delay_ticks, int waitAfterDelay) {
  int packetSize = 0;
  last_command_millis = millis();
  boolean responseExpected = true;
  lastCommandOK = true;
  display.clearDisplay();
  display.setCursor(0, 0);
  digitalWrite(COMMAND_TICK, LOW);
  Serial.println(command);
  display.println("Command:");
  display.println(command);
  display.display();
  // Special delay cases
  if (command.indexOf("takeoff") >= 0) udp_delay_ticks = 40;
  if (command.indexOf("land") >= 0) udp_delay_ticks = 20;
  if (command.indexOf("rc ") >= 0) {
    udp_delay_ticks = 0;
    responseExpected = false;
    lastCommandOK = true;  // rc commands assumed to be OK
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
      bool parseResponse = (commandResponse.indexOf("forced stop") == -1)
                           && (commandResponse.indexOf("error") == -1)
                           && (commandResponse.indexOf("ok") == -1);
      if (command.equalsIgnoreCase("battery?") && parseResponse) {
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
      } else if (commandResponse.indexOf("timeout") >= 0) {
        digitalWrite(COMMAND_TICK, LOW);
        Serial.println("Command timed out, ignoring for now");
      } else if (commandResponse.indexOf("forced stop") >= 0) {
        digitalWrite(COMMAND_TICK, LOW);
        Serial.println("Unexpected forced stop Response; treat as error");
        lastCommandOK = false;
      } else if (commandResponse.indexOf("error") >= 0) {
        digitalWrite(COMMAND_TICK, LOW);
        Serial.println("Unexpected error response; treat as error");
        lastCommandOK = false;
      }
    } else if (in_flight) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("No command response: ");
      display.println("Landing NOW!");
      display.display();
      lastCommandOK = false;
    }
  } else if (command.indexOf("command") >= 0) {  // no response
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("No command response: ");
    display.println("Restarting");
    display.display();
    lastCommandOK = false;
  }

  delay(waitAfterDelay);
}

void run_flight_plan() {
  if (connected) {
    Serial.println("run_flight_plan");
    run_command("motoron", 10, 5000);
    run_command("motoroff", 10, 1000);
    run_command("takeoff", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, HIGH);
    run_command("up 70", 20, 2000);
    run_command("cw 90", 20, 2000);
    run_command("ccw 90", 20, 2000);
    run_command("down 70", 20, 2000);
    run_command("land", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
  }
}

void run_flight_plan_A() {
  if (connected) {
    Serial.println("run_flight_plan_A");
    run_command("motoron", 10, 5000);
    run_command("motoroff", 10, 1000);
    run_command("takeoff", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, HIGH);
    run_command("up 70", 20, 2000);
    run_command("forward 70", 20, 2000);
    run_command("left 70", 20, 2000);
    run_command("back 70", 20, 2000);
    run_command("right 70", 20, 2000);
    run_command("cw 90", 20, 2000);
    run_command("ccw 90", 20, 2000);
    run_command("land", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
  }
}

void run_CCW_circle_flight() {
  Serial.println("run_CCW_circle_flight");
  if (connected) {
    run_command("takeoff", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, HIGH);
    run_command("rc 0 0 0 0", 0, 1000);
    run_command("rc 0 50 0 -100", 0, 4000);  // fly a CCW circular arc
    run_command("rc 0 0 0 0", 0, 1000);
    run_command("land", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
  }
}

void run_CW_circle_flight() {
  Serial.println("run_CW_circle_flight");
  if (connected) {
    run_command("takeoff", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, HIGH);
    run_command("rc 0 0 0 0", 0, 1000);
    run_command("rc 0 50 0 100", 0, 4000);  // fly a CW circular arc
    run_command("rc 0 0 0 0", 0, 1000);
    run_command("land", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
  }
}

void onTakeoffButtonPressed(Button2 &btn) {
  Serial.println("Takeoff button is pressed");
  if (in_flight) {
    run_command("land", 20, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
    in_flight = false;
  } else {
    run_command("takeoff", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, HIGH);
    in_flight = true;
  }
}

void onKillButtonPressed(Button2 &btn) {
  Serial.println("KILL button is pressed");
  if (!connected) {
    Serial.println("Kill Button Pressed, no connection, restarting");
    ESP.restart();
  }
  if (in_flight) {
    run_command("emergency", 10, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
    in_flight = false;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Emergency");
    display.println("Restarting!");
    Serial.println("Restarting: Emergency command ");
    delay(3000);
    ESP.restart();
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Kill Button Pressed");
    display.println("Changing MPU Status");
    if (use_MPUtoFly) {
      use_MPUtoFly = false;
      display.println("MPU Flying Disabled");
    } else {
      use_MPUtoFly = true;
      display.println("MPU Flying Enabled");
    }
    display.display();
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
          digitalWrite(IN_FLIGHT, HIGH);
          in_flight = true;
        } else if (command.indexOf("land") >= 0) {
          run_command(command, 40, 0);
          digitalWrite(IN_FLIGHT, LOW);
          in_flight = false;
        } else {
          run_command(command, 40, 0);
        }
      }
    }
  }
}
