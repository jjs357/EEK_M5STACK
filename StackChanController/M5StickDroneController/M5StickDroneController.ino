/*
 *  This sketch sends Tello commands over UDP from a ESP32 device
 *
 */

#include "DroneFunctions.h"
#include "DroneSetup.h"

// Wait until MiniJoyC is ready
static void waitMiniJoyCReady() {
  while (!joyc.begin(&Wire, MiniJoyC_ADDR, MiniJoyC_SDA, MiniJoyC_SCL, 100000UL)) {
    delay(100);
  }
  jsReady = 1;
}

void setup() {

  auto cfg = M5.config();
  M5.begin(cfg);
  waitMiniJoyCReady();

  pinMode(COMMAND_TICK, OUTPUT);
  digitalWrite(COMMAND_TICK, LOW);

  display.setRotation(0);
  display.fillScreen(BLACK);

  print_LED_Labels();
  virt_digitalWrite(conn, 0);
  virt_digitalWrite(flight, 0);

  display.setTextColor(TFT_WHITE, TFT_BLACK);
  clearTextLarge();
  display.println("Setup");
  display.println("Complete");
  delay(3000);
  animateVirtLEDs();

  clearText();

  digitalWrite(COMMAND_TICK, LOW);

  // Initilize hardware serial:
  Serial.begin(115200);
  // Serial.setTimeout(0);

  // Connect to drone WiFi
  manage_SSID_connection();
}

void loop() {
  check_serial();

  if (!lastCommandOK) {
    if (in_flight && connected) {
      Serial.println("Command Error: Attempt to Land");
      run_command("land", 40, 0);
      run_command("battery?", 30, 0);
      virt_digitalWrite(flight, 0);
      in_flight = false;
    }
    lastCommandOK = true;  // clear Command Error
  }

  M5.update();

  if (jsReady) {

    if (M5.BtnA.wasPressed()) {
      // Serial.println("Center button pressed");
      onTakeoffButtonPressed();
    }

    if (M5.BtnB.wasPressed()) {
      // Serial.println("Right side B button pressed");
      onBtnBPressed();
    }

    if (M5.BtnPWR.wasClicked()) {
      // Serial.println("Left side Power button pressed");
      onPowerButtonPressed();
    }

    if (joyc.getButtonStatus() == 0) {
      // Serial.println("Joystick Button pressed");
      if (!in_flight) {
        run_flight_plan_B();
      } else {
        onKillButtonPressed();
      }
    }

    imuValuesUpdate();

    Pitch = round(imuGetAngleX());
    Roll = round(imuGetAngleY());

    lastRcCmd = rcCmd;
    rcCmd = "rc 0 0 0 0";  // default is hover

    // process imu derived values before joystick values if useIMUtoFly is true
    if (in_flight) {  // process IMU values
      limitedRoll = 0;
      limitedPitch = 0;
      if (use_IMUtoFly) {
        AbsPitch = abs(Pitch);
        AbsRoll = abs(Roll);
        // set dead zone for Conttoller tilt angle
        if (AbsRoll <= 15) {
          limitedRoll = 0;
        } else {
          limitedRoll = map(AbsRoll, 15, 90, 20, 100);
          if (Roll > 15) {
            // negative sign to go left
            limitedRoll = -limitedRoll;
          }
        }
        // set dead zone for Controller tilt angle
        if (AbsPitch <= 15) {
          limitedPitch = 0;
        } else {
          limitedPitch = map(AbsPitch, 15, 90, 20, 100);
          if (Pitch > 15) {
            // negative sign to go back
            limitedPitch = -limitedPitch;
          }
        }
        if ((limitedRoll != 0) || (limitedPitch != 0)) {
          rcCmdHead = "rc ";
          rcCmdHead = rcCmdHead + limitedRoll + " " + limitedPitch + " ";
        } else {
          rcCmdHead = "rc 0 0 ";
        }
      }
      // Read normalized position (-128~127)
      int8_t pos_x = joyc.getPOSValue(POS_X, _8bit);
      int8_t pos_y = joyc.getPOSValue(POS_Y, _8bit);
      jsYaw = map((int)pos_x, -128, 127, -80, 80);
      jsThrottle = map((int)pos_y, -128, 127, -80, 80);
      // experimental dead zone values
      if (abs(jsYaw) <= 15) {
        jsYaw = 0;
      }
      if (abs(jsThrottle) <= 15) {
        jsThrottle = 0;
      }
      if ((jsThrottle != 0) || (jsYaw != 0)) {  // use joystick values if some are non-zero
        if (!use_IMUtoFly) {
          rcCmdHead = "rc 0 0 ";  // roll and pitch are zero
        }
        rcCmd = rcCmdHead + jsThrottle + " " + jsYaw;  // roll and or pitch may be non zero
      } else {
        rcCmd = rcCmdHead + "0 0";  // no throttle or yaw
      }
    }
    if (lastRcCmd != rcCmd) {
      run_command(rcCmd, 0, 0);
    }
  }

  if ((millis() - last_command_millis) > 15000) {  // keep SDK alive
    if (connected) run_command("battery?", 10, 0);
  }

  //  vTaskDelay(1);
}
