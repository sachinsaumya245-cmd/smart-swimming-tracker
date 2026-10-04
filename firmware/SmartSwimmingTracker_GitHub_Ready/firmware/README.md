# Firmware

This folder contains the ESP32 firmware for the Smart Swimming Tracker.

## Platform

The project is built with PlatformIO for ESP32.

## Setup

1. Open `SwimTracker_Blynk_MAX30102` in VS Code with PlatformIO installed.
2. Copy `src/secrets.example.h` to `src/secrets.h`.
3. Fill in your own Blynk token, Wi-Fi SSID, and Wi-Fi password in `src/secrets.h`.
4. Build and upload the project to the ESP32.

`src/secrets.h` is ignored by Git and should never be committed.
