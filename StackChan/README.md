# StackChan Overview

This file provides guidance to users when working with code in this folder.

## Introduction

The capabilities of the StackChan device and its included hardware are quite impressive. The EEK was originally targeted at the Tello drone but the product has been discontinued. As part of the EEK educational experience, a Tello Drone simulator was created using the Arduino IDE and implemented using a so-called Cheap Yellow Display (CYD) ESP32 device with a 320x240 TFT display.

The display of the StackChan has the same dimensions and with very minor coding changes the Tello simulator was modified and now runs on the StackChan.

While controlling a drone is more dynamic than controlling the servo motors that provide X and Y axis rotation for the StackChan, a similar coding approach to that taken to interact with the simulator can control the StackChan's tilt and circular rotations. The code in the StackChanRemoteController folder implements a remote-control mechanism using WiFi and UDP client/server principles.