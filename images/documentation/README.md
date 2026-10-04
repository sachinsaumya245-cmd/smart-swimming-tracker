# Project Documentation

This folder contains technical documentation for the Smart Swimming Tracker project.

The project is an embedded IoT wearable designed to monitor swimming performance using motion sensing, heart-rate monitoring, real-time data processing, and cloud connectivity.

---

## System Overview

The Smart Swimming Tracker is built around an ESP32 microcontroller.

The ESP32 collects data from:

- MPU6050 accelerometer and gyroscope
- MAX30102 optical heart-rate sensor
- battery monitoring circuit

The system processes sensor data to estimate swimming performance metrics such as:

- stroke count
- lap count
- current speed
- average speed
- heart rate

Selected values are transmitted to a Blynk IoT dashboard for real-time monitoring.

---

## System Architecture

```text
          MPU6050
      Accelerometer +
         Gyroscope
             |
             | I2C
             |
             v
        +----------+
        |          |
        |  ESP32   |
        |          |
        +----------+
          |      |
          |      |
          |      +-----------------> Blynk IoT Dashboard
          |
          +------------------------> Data Processing
          |                           - Stroke detection
          |                           - Lap detection
          |                           - Speed calculation
          |                           - Session statistics
          |
          +<------------------------- MAX30102
                                      Heart-rate sensor
