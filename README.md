# Smart Swimming Tracker

An embedded IoT wearable designed to monitor swimming performance using motion and heart-rate sensors.

## Project Overview

This project is a wearable swimming performance tracker built using an ESP32, MPU6050 IMU, and MAX30102 optical heart-rate sensor.

The system is designed to collect and process swimmer activity data in real time and send key metrics to an IoT dashboard.

## Main Features

- Stroke detection
- Lap counting
- Current speed estimation
- Average speed calculation
- Heart-rate monitoring
- Sensor calibration
- Motion analysis using MPU6050
- IoT data monitoring using Blynk
- Battery-powered wearable design

## Hardware

- ESP32
- MPU6050 accelerometer and gyroscope
- MAX30102 heart-rate sensor
- Li-Po battery
- TP4056 battery charging module
- DC-DC boost converter
- Push button for session reset
- Power switch
- Custom waterproof enclosure

## Communication

The MPU6050 and MAX30102 communicate with the ESP32 using the I2C communication protocol.

Sensor data is processed by the ESP32 and selected swimming metrics are transmitted to the Blynk IoT platform.

## Data Processing

The system processes motion data from the MPU6050 to identify swimming movements.

Current functionality includes:

- stroke recognition
- lap detection
- movement analysis
- speed estimation

The MAX30102 is used to monitor heart rate during a swimming session.

## Engineering Challenges

Some of the main challenges explored during development include:

- sensor calibration
- avoiding false stroke detections
- detecting laps reliably
- sensor orientation changes
- waterproof enclosure design
- battery and power management
- combining multiple sensors on the same I2C bus

## Technologies Used

- C / C++
- ESP32
- Embedded Systems
- IoT
- I2C
- Sensors
- Blynk
- Signal Processing

## Future Improvements

- Improve stroke classification accuracy
- Implement better sensor fusion
- Improve low-power operation
- Develop a custom PCB
- Add Bluetooth Low Energy communication
- Create a dedicated mobile application
- Improve swimmer performance analytics

## Project Status

Currently under development and testing.

## Author

**Sachin Saumya**  
Electrical & Electronic Engineering Undergraduate  
University of Peradeniya

Interested in embedded systems, sensors, IoT, mobile-device hardware and smartphone R&D.
