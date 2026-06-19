# EEK Controller Overview

This file provides guidance to users when working with code in this folder.

## Introduction

EEK hardware can control the StackChan when its firmware has been modified to act as a Tello Drone simulator. The EEK can also access selected hardware features of the StackChan including its display, servo motor control and the built in camera.

Two control Arduino programs are included for the EEK to interact with each firmware version that is installed on the StackChan. These are both based on Drone Control code that was created to fly an actual Tello. The same code can fly the simulator or a real Tello.