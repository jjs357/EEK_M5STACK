# Cheap Yellow Display Overview

This file provides guidance to users when working with code in this folder.

## Introduction

The StackChan software was inspired by the Drone Simulator software originally developed to run on the CYD: Cheap Yellow Display. The EEK was originally targeted at the Tello drone but the product has been discontinued. As part of the EEK educational experience, a Tello Drone simulator was created using the Arduino IDE and implemented using the Cheap Yellow Display (CYD) ESP32 device with a 320x240 TFT display.

The CYD includes both a display and an ESP32 processor. There are no real LEDs or programmable buttons as there are on the EEK. Virtual LEDs, small rectangular pixel areas, can be enabled to stand in for real LEDs. They are capable of showing a Pulse Width Modulation (PWM) effect where the brightnes of the pixel squares is tied to the strength of the GPIO signal pin whose value is sent from the EEK to the CYD Simulator.

The code in the TelloSimCYDPWM project folder implements a remote-control simulation capability using WiFi and UDP client/server principles. This code was used as the basis for the Drone Simulator running on the StackChan as you can see by comparing the two source files side by side. There were few code changes needed to adapt the CYD Drone Simulator to the StackChan hardware. The CYD screen is arranged as a portrait display: 240x320 and the pixel areas are bigger because the pixels on the CYD are not as tightly packed. The Arduino library ```TFT_eSPI``` is used instead of the StackChan library.

The following picture shows the CYD Drone Simulator display.

<img src="../Images/CYD-Sim-Fresh.png" width="50%">

Unlike the StackChan, the CYD is relatively inexpensive: under $30 from sources like Amazon. Although it has no kinetic capabilities, and no built-in battery, its UI showing Drone control effects is very similar to the StackChan drone controller UI. Since there is no battery on the CYD for the simulator to read, the CYD simulates a Tello Battery whose capacity fades over time from when the simulator is first started.