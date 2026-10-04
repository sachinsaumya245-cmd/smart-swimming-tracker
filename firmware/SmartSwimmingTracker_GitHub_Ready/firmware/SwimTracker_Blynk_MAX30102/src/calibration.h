#pragma once

class IMU;
class SwimTracker;
class Storage;

using CalibrationStatusCallback = void (*)(const char*);
using CalibrationServiceCallback = void (*)();

// Runs the 3-phase calibration sequence.
// Blocking overall, but serviceCb is called repeatedly so Wi-Fi/Blynk
// can stay alive while calibration is in progress.
bool runCalibration(IMU& imu,
                    SwimTracker& tracker,
                    Storage& store,
                    CalibrationStatusCallback statusCb = nullptr,
                    CalibrationServiceCallback serviceCb = nullptr);
