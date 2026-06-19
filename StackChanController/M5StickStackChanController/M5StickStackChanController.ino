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

  display.setRotation(0);
  display.fillScreen(BLACK);

  print_LED_Labels();
  virt_digitalWrite(conn, 0);
  virt_digitalWrite(control, 0);

  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.setCursor(0, 0);
  display.setFont(&fonts::AsciiFont8x16);
  //  display.setTextSize(2);
  //  display.setFont(&fonts::FreeMonoBold8pt7b);
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
          limitedRoll = map(AbsRoll, 15, 90, 400, 1280);
          if (Roll < 15) {
            // negative sign to go left
            limitedRoll = -limitedRoll;
          }
        }
        // set dead zone for Controller tilt angle
        if (AbsPitch <= 15) {
          limitedPitch = 0;
        } else {
          limitedPitch = map(AbsPitch, 15, 90, 300, 900);
          if (Pitch < 15) {
            // negative sign to go back
            limitedPitch = -limitedPitch;
          }
        }
        if ((limitedRoll != 0) || (limitedPitch != 0)) {
          if ((millis() - rcTime) > rcTimerLimit) {  // wait to use IMU values
            (AbsPitch > AbsRoll) ? rcCmd = "moveY " + String(limitedPitch) + " 500" : rcCmd = "moveX " + String(limitedRoll) + " 500";
            rcTime = millis();
          }
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


      if ((jsYangle != 0) || (jsXangle != 0)) {    // use joystick values if some are non-zero
        if ((millis() - rcTime) > rcTimerLimit) {  // wait to use Joystick values
          rcCmd = rcCmdHead + jsXangle + " " + jsYangle;
          rcTime = millis();
        }
      }
      if (lastRcCmd != rcCmd) {
        run_command(rcCmd, 40, 0);
        // Serial.println(gpadCommand);
      }
    }
  }

  //  vTaskDelay(1);
}