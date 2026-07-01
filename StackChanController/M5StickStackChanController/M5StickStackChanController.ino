/*
 *  This sketch sends Tello commands over UDP from a ESP32 device
 *
 */

#include "ControlFunctions.h"
#include "ControlSetup.h"

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

  cameraOn = false;
  
  display.setRotation(0);
  display.fillScreen(BLACK);

  print_LED_Labels();
  virt_digitalWrite(conn, 0);
  virt_digitalWrite(control, 0);

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

  M5.update();

  if (jsReady) {

    if (M5.BtnA.wasPressed()) {
      // Serial.println("Center button pressed");
      // onRotateButtonPressed();
      onTakeoffButtonPressed();
    }

    if (M5.BtnB.wasPressed()) {
      // Serial.println("Right side button pressed");
      // onTakeoffButtonPressed();
      onRotateButtonPressed();
    }

    if (M5.BtnPWR.wasClicked()) {
      onPowerButtonPressed();
    }

    if (joyc.getButtonStatus() == 0) {
      // Serial.println("Joystick Button pressed");
      if (!in_control) {
        set_home_manually();
      } else {
        run_command("move 0 0", 20, 0);
      }
    }

    imuValuesUpdate();

    Pitch = round(imuGetAngleX());
    Roll = round(imuGetAngleY());

    lastRcCmd = rcCmd;
    // rcCmd = "stop";  // default is hover

    // process imu derived values before joystick values if useIMUtoFly is true
    if (in_control) {  // To-Do process IMU values

      limitedRoll = 0;
      limitedPitch = 0;
      if (use_IMUtoControl) {
        AbsPitch = abs(Pitch);
        AbsRoll = abs(Roll);
        // set dead zone for Conttoller tilt angle
        if (AbsRoll <= 15) {
          limitedRoll = 0;
        } else {
          limitedRoll = (AbsRoll >= 60) ? 900 : (AbsRoll >= 40) ? 600
                                                                : 300;
        }
        // set dead zone for Controller tilt angle
        if (AbsPitch <= 15) {
          limitedPitch = 0;
        } else {
          limitedPitch = (AbsPitch >= 60) ? 450 : (AbsPitch >= 30) ? 300
                                                                   : 150;
        }

        if ((millis() - imuTime) > imuTimerLimit) {  // wait to use IMU values
          if ((limitedRoll != 0) || (limitedPitch != 0)) {
            if (Pitch > 0) Yangle = min(Yangle + limitedPitch, 900);
            else Yangle = max(Yangle - limitedPitch, 0);
            if (Roll < 0) Xangle = max(Xangle - limitedRoll, -1280);
            else Xangle = min(Xangle + limitedRoll, 1280);
            // (AbsPitch > AbsRoll) ? rcCmd = "moveY " + String(limitedPitch) + " 500" : rcCmd = "moveX " + String(limitedRoll) + " 500";
            rcCmd = rcCmdHead + Xangle + " " + Yangle;
          }
          imuTime = millis();
        }
        if (lastRcCmd != rcCmd) {
          run_command(rcCmd, 40, 0);
          // Serial.println(gpadCommand);
        }
      }

      // Read normalized position (-128~127)
      int8_t pos_x = joyc.getPOSValue(POS_X, _8bit);
      int8_t pos_y = joyc.getPOSValue(POS_Y, _8bit);
      jsXangle = map((int)pos_x, -128, 127, 1280, -1280);
      jsYangle = map((int)pos_y, -128, 127, -800, 800);
      // experimental dead zone values
      if (abs(jsXangle) <= 150) {
        jsXangle = 0;
      }
      if (abs(jsYangle) <= 150) {
        jsYangle = 0;
      }

      if ((millis() - jsTime) > jsTimerLimit) {    // wait to use Joystick values
        if ((jsYangle != 0) || (jsXangle != 0)) {  // use joystick values if some are non-zero
          Yangle = (jsYangle < 0) ? Yangle + jsYangle : jsYangle;
          if (jsXangle > 0) {
            Xangle = (Xangle < 0) ? Xangle + jsXangle : jsXangle;
          } else {
            Xangle = (Xangle < 0) ? jsXangle : Xangle + jsXangle;
          }
          rcCmd = rcCmdHead + Xangle + " " + Yangle;
        }
        jsTime = millis();
        if (lastRcCmd != rcCmd) {
          run_command(rcCmd, 40, 0);
          // Serial.println(gpadCommand);
        }
      }
    }
  }

  //  vTaskDelay(1);
}