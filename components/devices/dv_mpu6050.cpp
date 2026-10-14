#pragma once

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct Vector3 {
    float x, y, z;
};

struct Vector3G {
    float x, y, z;
};

struct MPU6050Data {
    Vector3G accelerationG;
    Vector3 acceleration; // m/s^2
    Vector3G gyroDPS;     // Grados por segundo
    Vector3 gyro;         // Radianes por segundo
};

class MPU6050Calibration {
private:
    Vector3 accelOffset;
    Vector3 accelScale;
    Vector3 gyroOffset;

public:
    MPU6050Calibration();
    void setAccelOffset(float x, float y, float z);
    void setAccelScale(float x, float y, float z);
    void setGyroOffset(float x, float y, float z);
    Vector3 getAccelOffset();
    Vector3 getAccelScale();
    Vector3 getGyroOffset();
    Vector3 compensateAcceleration(Vector3 raw);
    Vector3 compensateGyro(Vector3 raw);
};

class MPU6050 {
private:
    i2c_port_t i2c_port;
    uint8_t address;
    int16_t rawAx, rawAy, rawAz;
    int16_t rawGx, rawGy, rawGz;
    MPU6050Calibration calibration;

    const float ACCEL_SENSITIVITY = 16384.0f; // Para ±2g
    const float GYRO_SENSITIVITY = 131.0f;    // Para ±250 deg/s
    const float GRAVITY = 9.80665f;
    const float DEG_TO_RAD = 0.01745329252f;
    const uint16_t SAMPLE_DELAY_MS = 10;

    const uint8_t REG_PWR_MGMT1 = 0x6B;
    const uint8_t REG_SMPLRT_DIV = 0x19;
    const uint8_t REG_CONFIG = 0x1A;
    const uint8_t REG_GYRO_CONFIG = 0x1B;
    const uint8_t REG_ACCEL_CONFIG = 0x1C;
    const uint8_t REG_WHO_AM_I = 0x75;
    const uint8_t REG_ACCEL_XOUT_H = 0x3B;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
    Vector3 calibratePosition(char axis, int direction, uint16_t samples);

public:
    MPU6050(i2c_port_t port, uint8_t addr = 0x68);
    bool begin(int sda_pin, int scl_pin, uint32_t clk_speed = 400000);
    bool testConnection();
    bool readRaw();
    void calibrate(uint16_t samples);
    bool read(MPU6050Data& data);
    void printCalibration();
};