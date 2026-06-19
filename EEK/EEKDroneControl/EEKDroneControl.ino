/*
 *  This sketch can talk to a Tello and uses MPU6050 for gesture control
 *  Takeoff and Kill buttons are handled via Button2 library.
 */
#include "DroneFunctions.h"
#include "DroneSetup.h"

void setup() {

  Wire.begin();

  // EEK Buttons are active high and some are INPUT_ONLY (no internal pull-up/down)
  // Appropriate declaration for active high with external pull-down is begin(pin, INPUT, false)
  takeoffButton.begin(TAKEOFF_PIN, INPUT, false);
  takeoffButton.setTapHandler(onTakeoffButtonPressed);

  killButton.begin(KILL_PIN, INPUT, false);
  killButton.setTapHandler(onKillButtonPressed);

  pinMode(UP_PIN, INPUT);
  pinMode(CW_PIN, INPUT);
  pinMode(CCW_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);

  pinMode(LED_CONN_RED, OUTPUT);
  pinMode(LED_CONN_GREEN, OUTPUT);
  pinMode(LED_BATT_RED, OUTPUT);
  pinMode(LED_BATT_YELLOW, OUTPUT);
  pinMode(LED_BATT_GREEN, OUTPUT);
  pinMode(COMMAND_TICK, OUTPUT);
  pinMode(IN_FLIGHT, OUTPUT);

  // Initialize hardware serial
  Serial.begin(115200);
  // Serial.setTimeout(0);

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.display();
  delay(2000);  // Pause for 2 seconds
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setRotation(0);
  display.clearDisplay();
  display.setCursor(0, 0);

  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  display.println("MPU6050 status: ");
  Serial.println(status);
  display.println(status);
  display.display();

  while (status != 0) {}  // stop everything if could not connect to MPU6050

  Serial.println(F("Calculating offsets, do not move MPU6050"));
  display.println("Hold EEK still");
  mpu.calcOffsets();  // gyro and accelero
  Serial.println("Done!\n");
  display.println("Done!");
  display.display();
  delay(2000);

  animateEEKLEDs();
  digitalWrite(LED_CONN_RED, HIGH);
  use_MPUtoFly = false;
  use_SDK_KeepAlive = true;

  int rawValue = analogRead(VBATPIN);
  float voltageLevel = (rawValue / 4095.0) * 2 * 1.1 * 3.3;  // calculate voltage level
  int batteryFraction = min(100, int(trunc(voltageLevel / MAX_BATTERY_VOLTAGE * 100)));
  Serial.print("Controller Battery %: ");
  Serial.println(batteryFraction);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Controller Batt %:");
  display.println(batteryFraction);
  display.display();
  delay(2000);

  manage_SSID_connection();
}

void loop() {
  check_serial();

  if (!lastCommandOK) {
    if (in_flight && connected) {
      Serial.println("Command Error: Attempt to Land");
      run_command("land", 40, 0);
      digitalWrite(IN_FLIGHT, LOW);
      in_flight = false;
    }
    lastCommandOK = true;  // clear Command Error
  }

  upState = digitalRead(UP_PIN);
  downState = digitalRead(DOWN_PIN);
  cwState = digitalRead(CW_PIN);
  ccwState = digitalRead(CCW_PIN);

  takeoffButton.loop();
  killButton.loop();

  mpu.update();
  Roll = mpu.getAngleX();
  Pitch = mpu.getAngleY();
  Yaw = mpu.getAngleZ();

  lastRcCmd = rcCmd;
  rcCmd = "rc 0 0 0 0";  // default is hover

  if (in_flight) {
    if (upState == HIGH) {
      rcCmd = "rc 0 0 75 0";
    } else if (downState == HIGH) {
      rcCmd = "rc 0 0 -75 0";
    } else if (cwState == HIGH) {
      rcCmd = "rc 0 0 0 75";
    } else if (ccwState == HIGH) {
      rcCmd = "rc 0 0 0 -75";
    } else {  // no buttons pressed
      if (use_MPUtoFly) {
        AbsPitch = abs(Pitch);
        AbsRoll = abs(Roll);
        // set dead zone for EEK tilt angle
        if (AbsRoll <= 10) {
          limitedRoll = 0;
        } else {
          AbsRoll = constrain(AbsRoll, 10, 40);
          limitedRoll = (AbsRoll >= 40) ? 90 : (AbsRoll >= 25) ? 70
                                                               : 50;
          if (Roll > 10) {
            // negative sign to go left
            limitedRoll = -limitedRoll;
          }
        }
        // set dead zone for EEK tilt angle
        if (AbsPitch <= 15) {
          limitedPitch = 0;
        } else {
          AbsPitch = constrain(AbsPitch, 15, 45);
          limitedPitch = (AbsPitch >= 45) ? 90 : (AbsPitch >= 30) ? 70
                                                                  : 50;
          if (Pitch > 15) {
            // negative sign to go back
            limitedPitch = -limitedPitch;
          }
        }
        if ((limitedRoll != 0) || (limitedPitch != 0)) {
          rcCmd = "rc ";
          rcCmd = rcCmd + limitedRoll + " " + limitedPitch + " 0 0";
        }
      }
    }
    if (lastRcCmd != rcCmd) {
      lastCommand = lastRcCmd;
      appendLastCommand();
      run_command(rcCmd, 0, 0);
      //      Serial.println(goCommand);
    } else {
      lastCommand = "rc 0 0 0 0";  //default last command: hover
    }

  } else {  // not in_flight
    if (upState == HIGH) {
      Serial.println("Up button is pressed when not in flight");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Up Button Pressed");
      display.println("Running Flight Plan");
      display.display();
      delay(2000);
      run_flight_plan();
    } else if (downState == HIGH) {
      Serial.println("Down button is pressed when not in flight");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Down Button Pressed");
      display.println("Changing MPU Status");
      if (use_MPUtoFly) {
        use_MPUtoFly = false;
        display.println("MPU Flying Disabled");
      } else {
        use_MPUtoFly = true;
        display.println("MPU Flying Enabled");
      }
      display.display();
      delay(2000);
    } else if (ccwState == HIGH) {
      // Serial.println("CCW button is pressed when not in flight");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("CCW Button Pressed");
      display.println("Changing Keep Alive");
      display.println("Status");
      if (use_SDK_KeepAlive) {
        use_SDK_KeepAlive = false;
        display.println("Keep Alive Disabled");
      } else {
        use_SDK_KeepAlive = true;
        display.println("Keep Alive Enabled");
      }
      display.display();
      delay(2000);
    }
  }

  if (use_SDK_KeepAlive && ((millis() - last_command_millis) > 15000)) {  // keep SDK alive
    if (connected) run_command("battery?", 10, 0);
  }
}
