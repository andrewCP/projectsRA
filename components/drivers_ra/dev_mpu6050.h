#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

#define MPU6050_ADDRESS 0x68
#define CALIBRATION_SAMPLES 1500
#define SAMPLE_DELAY_MS 10

const float GRAVITY = 9.80665f;
const float ACCEL_SENSITIVITY = 16384.0f;
const float GYRO_SENSITIVITY = 131.0f;
const float DEG_TORAD = 3.14159265f / 180.0f;

struct Vector3
{
    float x;
    float y;
    float z;
};

struct MPU6050Data
{
    Vector3 acceleration;
    Vector3 accelerationG;
    Vector3 gyro;
    Vector3 gyroDPS;
};

class MPU6050Calibration
{
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

class MPU6050
{
private:
    TwoWire* i2c;
    uint8_t address;
    MPU6050Calibration calibration;

    static constexpr uint8_t REG_SMPLRT_DIV = 0x19;
    static constexpr uint8_t REG_CONFIG = 0x1A;
    static constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
    static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
    static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
    static constexpr uint8_t REG_PWR_MGMT1 = 0x6B;
    static constexpr uint8_t REG_WHO_AM_I = 0x75;

    int16_t rawAx, rawAy, rawAz;
    int16_t rawGx, rawGy, rawGz;

    void writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);

public:
    MPU6050(TwoWire& wire = Wire, uint8_t addr = MPU6050_ADDRESS);
    bool begin(uint8_t SDA = 21, uint8_t SCL = 22);
    bool testConnection();
    bool readRaw();
    Vector3 calibratePosition(char axis, int direction, uint16_t samples);
    void calibrate(uint16_t samples = CALIBRATION_SAMPLES);
    bool read(MPU6050Data& data);
    void printCalibration();
};

#endif