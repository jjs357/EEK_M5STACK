# StackChan Controller Overview

This file provides guidance to users when working with code in this repository.

## Introduction

The StackChan M5Stick controller firmware can be replaced with Arduino-based software where its IMU and Joystick can control Servo motion wirelessly from the M5Stick StackChan controller that is optionally included with the StackChan hardware or by the EEK. A similar approach allows the StackChan Controller to interact with the Tello Simulator running on the StackChan as well as control a real Tello.

Two control Arduino programs are included for the StackChan controller to interact with each firmware version that is installed on the StackChan. These are both based on Drone Control code that was created to fly an actual Tello. The drone controller code can fly the simulator or a real Tello.