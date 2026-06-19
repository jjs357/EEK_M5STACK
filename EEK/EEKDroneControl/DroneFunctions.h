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

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void appendLastCommand() {
  this_since_takeoff = (millis() - takeoff_time);
  commandDelay = this_since_takeoff - last_since_takeoff;
  lastCommand = lastCommand + "," + commandDelay + "\n";
  appendFile(SPIFFS, flightFilePath, lastCommand.c_str());
  last_since_takeoff = this_since_takeoff;
}

void processCommand(String command) {
  appendLastCommand();
  run_command(command, 0, 0);
  lastCommand = command;
}

void run_flight_plan() {
  Serial.println("run_flight_plan");
  if (connected) {
    run_command("motoron", 10, 5000);
    run_command("motoroff", 10, 1000);
    run_command("takeoff", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, HIGH);
    run_command("rc 0 0 0 0", 0, 1000);
    run_command("rc 0 50 0 -100", 0, 4000);  // fly a CCW circular arc
    run_command("rc 0 0 0 0", 0, 1000);
    run_command("land", 40, 0);
    if (lastCommandOK) digitalWrite(IN_FLIGHT, LOW);
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
    digitalWrite(IN_FLIGHT, LOW);
    in_flight = false;
    deleteFile(SPIFFS, flightFilePath);
  } else {
    // run saved flight file if there is one
    flightFile = SPIFFS.open(flightFilePath, FILE_READ);
    if (flightFile) {
      Serial.println("Start of Flight File...");
      while (flightFile.available()) {
        String command = flightFile.readStringUntil('\n');
        int commaPosition = command.indexOf(',');
        if (commaPosition != -1) {  // delay value present
          commandDelay = command.substring(commaPosition + 1, command.length()).toInt();
          // Serial.println(command);
          // Serial.println(command.substring(0, commaPosition));
          // Serial.println(commandDelay);
          run_command(command.substring(0, commaPosition), 20, commandDelay);
        } else {
          run_command(command, 40, 0);
          // Serial.println(command);
        }
        if (command.indexOf("takeoff") >= 0) {
            digitalWrite(IN_FLIGHT, HIGH);
            in_flight = true;
          } else if (command.indexOf("land") >= 0) {
            digitalWrite(IN_FLIGHT, LOW);
            in_flight = false;
          }
        // Serial.println(command);
      }
      Serial.println("... end of Flight File");
      flightFile.close();
    }
  }
}

void onTakeoffButtonPressed(Button2 &btn) {
  Serial.println("Takeoff button is pressed");
  if (in_flight) {
    appendLastCommand();
    run_command("land", 20, 0);
    appendFile(SPIFFS, flightFilePath, "land,2\n");
    digitalWrite(IN_FLIGHT, LOW);
    in_flight = false;
  } else {
    writeFile(SPIFFS, flightFilePath, "command,2\n");
    appendFile(SPIFFS, flightFilePath, "battery?,2\n");
    run_command("takeoff", 40, 0);
    digitalWrite(IN_FLIGHT, HIGH);
    takeoff_time = millis();
    last_since_takeoff = 0;
    in_flight = true;
    lastCommand = "takeoff";
    appendLastCommand();  // will be takeoff
  }
}
