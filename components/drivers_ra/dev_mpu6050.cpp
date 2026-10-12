#include "dev_mpu6050.h"

// --- MPU6050Calibration Implementation ---
MPU6050Calibration::MPU6050Calibration() {
    accelOffset = {0.0f, 0.0f, 0.0f};
    accelScale = {1.0f, 1.0f, 1.0f};
    gyroOffset = {0.0f, 0.0f, 0.0f};
}

void MPU6050Calibration::setAccelOffset(float x, float y, float z) {
    accelOffset.x = x; accelOffset.y = y; accelOffset.z = z;
}

void MPU6050Calibration::setAccelScale(float x, float y, float z) {
    accelScale.x = x; accelScale.y = y; accelScale.z = z;
}

void MPU6050Calibration::setGyroOffset(float x, float y, float z) {
    gyroOffset.x = x; gyroOffset.y = y; gyroOffset.z = z;
}

Vector3 MPU6050Calibration::getAccelOffset() { return accelOffset; }
Vector3 MPU6050Calibration::getAccelScale() { return accelScale; }
Vector3 MPU6050Calibration::getGyroOffset() { return gyroOffset; }

Vector3 MPU6050Calibration::compensateAcceleration(Vector3 raw) {
    return {
        (raw.x - accelOffset.x) * accelScale.x,
        (raw.y - accelOffset.y) * accelScale.y,
        (raw.z - accelOffset.z) * accelScale.z
    };
}

Vector3 MPU6050Calibration::compensateGyro(Vector3 raw) {
    return {
        raw.x - gyroOffset.x,
        raw.y - gyroOffset.y,
        raw.z - gyroOffset.z
    };
}

// --- MPU6050 Implementation ---
MPU6050::MPU6050(TwoWire& wire, uint8_t addr) {
    i2c = &wire;
    address = addr;
    rawAx = rawAy = rawAz = 0;
    rawGx = rawGy = rawGz = 0;
}

void MPU6050::writeRegister(uint8_t reg, uint8_t value) {
    i2c->beginTransmission(address);
    i2c->write(reg);
    i2c->write(value);
    i2c->endTransmission();
}

bool MPU6050::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
    i2c->beginTransmission(address);
    i2c->write(reg);
    if (i2c->endTransmission(false) != 0) return false;

    uint8_t received = i2c->requestFrom(address, length);
    if (received != length) return false;

    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = i2c->read();
    }
    return true;
}

bool MPU6050::begin(uint8_t SDA, uint8_t SCL) {
    i2c->begin(SDA, SCL);
    delay(100);
    writeRegister(REG_PWR_MGMT1, 0x00);
    delay(100);
    writeRegister(REG_SMPLRT_DIV, 9);
    writeRegister(REG_CONFIG, 0x04);
    writeRegister(REG_GYRO_CONFIG, 0x00);
    writeRegister(REG_ACCEL_CONFIG, 0x00);
    delay(100);
    return testConnection();
}

bool MPU6050::testConnection() {
    uint8_t whoAmI = 0;
    if (!readRegisters(REG_WHO_AM_I, &whoAmI, 1)) return false;
    return (whoAmI == 0x68);
}

bool MPU6050::readRaw() {
    uint8_t buffer[14];
    if (!readRegisters(REG_ACCEL_XOUT_H, buffer, 14)) return false;

    rawAx = ((int16_t)buffer[0] << 8) | buffer[1];
    rawAy = ((int16_t)buffer[2] << 8) | buffer[3];
    rawAz = ((int16_t)buffer[4] << 8) | buffer[5];
    rawGx = ((int16_t)buffer[8] << 8) | buffer[9];
    rawGy = ((int16_t)buffer[10] << 8) | buffer[11];
    rawGz = ((int16_t)buffer[12] << 8) | buffer[13];
    return true;
}

Vector3 MPU6050::calibratePosition(char axis, int direction, uint16_t samples) {
    float sumX = 0, sumY = 0, sumZ = 0;
    int validSamples = 0;

    Serial.println("\n--------------------------------");
    Serial.print("POSICION: ");
    if (axis == 'X') Serial.print(direction > 0 ? "+X" : "-X");
    if (axis == 'Y') Serial.print(direction > 0 ? "+Y" : "-Y");
    if (axis == 'Z') Serial.print(direction > 0 ? "+Z" : "-Z");
    Serial.println("\nColoque el eje indicado hacia ARRIBA.\nSensor completamente inmovil.\nIniciando en 5 segundos...");
    delay(5000);

    for (uint16_t i = 0; i < samples; i++) {
        if (readRaw()) {
            sumX += rawAx; sumY += rawAy; sumZ += rawAz;
            validSamples++;
        }
        delay(SAMPLE_DELAY_MS);
        if ((i + 1) % 100 == 0) {
            Serial.print("Muestras: "); Serial.print(i + 1); Serial.print("/"); Serial.println(samples);
        }
    }

    Vector3 average = {sumX / validSamples, sumY / validSamples, sumZ / validSamples};
    Serial.println("\nPromedios RAW:");
    Serial.print("X = "); Serial.println(average.x, 3);
    Serial.print("Y = "); Serial.println(average.y, 3);
    Serial.print("Z = "); Serial.println(average.z, 3);
    return average;
}

void MPU6050::calibrate(uint16_t samples) {
    Serial.println("\n========================================\n       CALIBRACION MPU6050\n       6 POSICIONES\n========================================");
    Serial.print("Muestras por posicion: "); Serial.println(samples);
    Serial.println("\nTOTAL DE POSICIONES: 6");
    Serial.print("TOTAL DE MUESTRAS: "); Serial.println(samples * 6);

    Vector3 plusX = calibratePosition('X', +1, samples);
    Vector3 minusX = calibratePosition('X', -1, samples);
    Vector3 plusY = calibratePosition('Y', +1, samples);
    Vector3 minusY = calibratePosition('Y', -1, samples);
    Vector3 plusZ = calibratePosition('Z', +1, samples);
    Vector3 minusZ = calibratePosition('Z', -1, samples);

    Vector3 offset = {(plusX.x + minusX.x) / 2.0f, (plusY.y + minusY.y) / 2.0f, (plusZ.z + minusZ.z) / 2.0f};
    float sensitivityX = (plusX.x - minusX.x) / 2.0f;
    float sensitivityY = (plusY.y - minusY.y) / 2.0f;
    float sensitivityZ = (plusZ.z - minusZ.z) / 2.0f;

    Vector3 scale = {
        ACCEL_SENSITIVITY / sensitivityX,
        ACCEL_SENSITIVITY / sensitivityY,
        ACCEL_SENSITIVITY / sensitivityZ
    };

    Serial.println("\n========================================\nCALIBRACION DEL GIROSCOPIO\nMantenga el sensor completamente quieto.\nTomando 1500 muestras...");
    delay(3000);

    float gyroX = 0, gyroY = 0, gyroZ = 0;
    int gyroSamples = 0;
    for (uint16_t i = 0; i < samples; i++) {
        if (readRaw()) {
            gyroX += rawGx; gyroY += rawGy; gyroZ += rawGz;
            gyroSamples++;
        }
        delay(SAMPLE_DELAY_MS);
    }

    Vector3 gyroOffset = {gyroX / gyroSamples, gyroY / gyroSamples, gyroZ / gyroSamples};

    calibration.setAccelOffset(offset.x, offset.y, offset.z);
    calibration.setAccelScale(scale.x, scale.y, scale.z);
    calibration.setGyroOffset(gyroOffset.x, gyroOffset.y, gyroOffset.z);

    printCalibration();
}

bool MPU6050::read(MPU6050Data& data) {
    if (!readRaw()) return false;

    Vector3 accelerationRaw = {(float)rawAx, (float)rawAy, (float)rawAz};
    Vector3 accelerationCorrected = calibration.compensateAcceleration(accelerationRaw);

    data.accelerationG.x = accelerationCorrected.x / ACCEL_SENSITIVITY;
    data.accelerationG.y = accelerationCorrected.y / ACCEL_SENSITIVITY;
    data.accelerationG.z = accelerationCorrected.z / ACCEL_SENSITIVITY;

    data.acceleration.x = data.accelerationG.x * GRAVITY;
    data.acceleration.y = data.accelerationG.y * GRAVITY;
    data.acceleration.z = data.accelerationG.z * GRAVITY;

    Vector3 gyroRaw = {(float)rawGx, (float)rawGy, (float)rawGz};
    Vector3 gyroCorrected = calibration.compensateGyro(gyroRaw);

    data.gyroDPS.x = gyroCorrected.x / GYRO_SENSITIVITY;
    data.gyroDPS.y = gyroCorrected.y / GYRO_SENSITIVITY;
    data.gyroDPS.z = gyroCorrected.z / GYRO_SENSITIVITY;

    data.gyro.x = data.gyroDPS.x * DEG_TO_RAD;
    data.gyro.y = data.gyroDPS.y * DEG_TO_RAD;
    data.gyro.z = data.gyroDPS.z * DEG_TO_RAD;

    return true;
}

void MPU6050::printCalibration() {
    Vector3 offset = calibration.getAccelOffset();
    Vector3 scale = calibration.getAccelScale();
    Vector3 gyro = calibration.getGyroOffset();

    Serial.println("\n========================================\n       RESULTADOS CALIBRACION\n========================================");
    Serial.println("\nACELEROMETRO\n-------------------------");
    Serial.print("Offset X: "); Serial.println(offset.x, 6);
    Serial.print("Offset Y: "); Serial.println(offset.y, 6);
    Serial.print("Offset Z: "); Serial.println(offset.z, 6);
    Serial.print("Scale X: "); Serial.println(scale.x, 8);
    Serial.print("Scale Y: "); Serial.println(scale.y, 8);
    Serial.print("Scale Z: "); Serial.println(scale.z, 8);
    Serial.println("\nGIROSCOPIO\n-------------------------");
    Serial.print("Offset X: "); Serial.println(gyro.x, 6);
    Serial.print("Offset Y: "); Serial.println(gyro.y, 6);
    Serial.print("Offset Z: "); Serial.println(gyro.z, 6);
    Serial.println("========================================");
}