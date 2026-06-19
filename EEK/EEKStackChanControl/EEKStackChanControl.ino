/*
 *  This sketch sends Tello commands over UDP from a ESP32 device
 *
 */

#include "ControlFunctions.h"
#include "ControlSetup.h"

void setup() {

  Wire.begin();

  // EEK Buttons are active high and some are INPUT_ONLY (no internal pull-up/down)
  // Appropriate declaration for active high with external pull-down is begin(pin, INPUT, false)
  takeoffButton.begin(TAKEOFF_PIN, INPUT, false);
  takeoffButton.setTapHandler(onTakeoffButtonPressed);

  upButton.begin(UP_PIN, INPUT, false);
  upButton.setTapHandler(onUpButtonTapped);
  upButton.setLongClickHandler(cameraClick);

  downButton.begin(DOWN_PIN, INPUT, false);
  downButton.setTapHandler(onDownButtonTapped);
  downButton.setLongClickHandler(cameraClick);

  cwButton.begin(CW_PIN, INPUT, false);
  cwButton.setTapHandler(onCWButtonPressed);

  ccwButton.begin(CCW_PIN, INPUT, false);
  ccwButton.setTapHandler(onCCWButtonPressed);

  killButton.begin(KILL_PIN, INPUT, false);
  killButton.setTapHandler(onKillButtonPressed);

  pinMode(LED_USING_MPU, OUTPUT);
  pinMode(LED_CONN_GREEN, OUTPUT);
  pinMode(LED_BATT_RED, OUTPUT);
  pinMode(LED_BATT_YELLOW, OUTPUT);
  pinMode(LED_BATT_GREEN, OUTPUT);
  pinMode(COMMAND_TICK, OUTPUT);
  pinMode(IN_CONTROL, OUTPUT);

  // Initialize hardware serial
  Serial.begin(115200);
  // Serial.setTimeout(0);

  // Initilize hardware serial:
  Serial.begin(115200);
  // Serial.setTimeout(0);

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
  digitalWrite(LED_USING_MPU, HIGH);
  use_MPUtoControl = false;

  // Connect to drone WiFi
  manage_SSID_connection();
}

void loop() {
  check_serial();

  takeoffButton.loop();
  upButton.loop();
  downButton.loop();
  cwButton.loop();
  ccwButton.loop();
  killButton.loop();

  mpu.update();
  Roll = mpu.getAngleX();
  Pitch = mpu.getAngleY();

  lastRcCmd = rcCmd;
  // rcCmd = "stop";  // default is hover

  // process imu derived values before joystick values if useIMUtoFly is true
  if (in_control) {  // To-Do process MPU values

    if (use_MPUtoControl) {
      limitedRoll = 0;
      limitedPitch = 0;
      AbsPitch = abs(Pitch);
      AbsRoll = abs(Roll);
      // set dead zone for Conttoller tilt angle
      if (AbsRoll <= 10) {
        limitedRoll = 0;
      } else {
        // limitedRoll = map(AbsRoll, 15, 90, 400, 1280);
        // AbsRoll = constrain(AbsRoll, 10, 40);
        limitedRoll = (AbsRoll >= 60) ? 1280 : (AbsRoll >= 40) ? 900
                                                               : 450;
        if (Roll < 10) {
          limitedRoll = -limitedRoll;
        }
      }
      // set dead zone for Controller tilt angle
      if (AbsPitch <= 15) {
        limitedPitch = 0;
      } else {
        // limitedPitch = map(AbsPitch, 15, 90, 300, 900);
        // AbsPitch = constrain(AbsPitch, 15, 45);
        limitedPitch = (AbsPitch >= 60) ? 900 : (AbsPitch >= 30) ? 450
                                                                 : 300;
        if (Pitch > 15) {
          limitedPitch = -limitedPitch;
        }
      }
      if ((millis() - rcTime) > rcTimerLimit) {  // wait to use IMU values
        if ((limitedRoll != 0) || (limitedPitch != 0)) {
          // (AbsPitch > AbsRoll) ? rcCmd = "moveY " + String(limitedPitch) + " 500" : rcCmd = "moveX " + String(limitedRoll) + " 500";
          rcCmd = rcCmdHead + limitedRoll + " " + limitedPitch;
          rcHomeTime = millis();
        }
        rcTime = millis();
      } else if ((millis() - rcHomeTime) > rcHomeTimerLimit) {
        rcCmd = "move 0 0";
        rcHomeTime = millis();
        rcTime = millis();
      }
    }

    if (lastRcCmd != rcCmd) {
      run_command(rcCmd, 40, 0);
      // Serial.println(gpadCommand);
    }
  }
}
