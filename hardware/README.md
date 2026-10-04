# Hardware

This folder contains the hardware documentation for the Smart Swimming Tracker.

## Main Components

- ESP32 development board
- MPU6050 accelerometer and gyroscope
- MAX30102 optical heart-rate sensor
- Li-Po battery
- TP4056 Li-Ion/Li-Po charging module with protection
- DC-DC boost converter
- Slide switch for power control
- Push button for session reset
- Waterproof enclosure

## Sensor Communication

Both the MPU6050 and MAX30102 communicate with the ESP32 using the I2C protocol.

### I2C Connections

- SDA → ESP32 GPIO 21
- SCL → ESP32 GPIO 22
- Common GND shared between all modules
- Sensor power supplied from the appropriate regulated voltage rail

## Power System

The system is powered by a rechargeable Li-Po battery.

The battery is connected through:

Li-Po Battery → TP4056 Charging/Protection Module → DC-DC Converter → ESP32 and Sensors

A slide switch is used as the main power switch.

## User Input

A push button is used for session reset/control.

## Mechanical Design

The electronics are housed inside a compact waterproof enclosure designed for swimming use.

Special attention was given to:

- sensor placement
- battery placement
- waterproofing
- charging-port access
- button access
- compact internal layout

## Future Hardware Improvements

- custom PCB
- smaller enclosure
- improved waterproof sealing
- optimized battery placement
- lower-power regulator
- improved MAX30102 skin contact
