#include "mpu6050.h"

#include <stdint.h>

#include "hardware/soft_i2c/soft_i2c.h"

#define MPU_I2C_ADDR0               0x68U
#define MPU_I2C_ADDR1               0x69U

#define MPU_REG_SMPLRT_DIV          0x19U
#define MPU_REG_CONFIG              0x1AU
#define MPU_REG_GYRO_CONFIG         0x1BU
#define MPU_REG_ACCEL_CONFIG        0x1CU
#define MPU_REG_ACCEL_XOUT_H        0x3BU
#define MPU_REG_PWR_MGMT_1          0x6BU
#define MPU_REG_WHO_AM_I            0x75U

#define MPU_ATTITUDE_ALPHA          0.985f
#define MPU_DEG_PER_RAD             57.29578f
#define MPU_PI                      3.1415926f

#define MPU_GYRO_PITCH_SIGN         1.0f
#define MPU_GYRO_ROLL_SIGN          1.0f
#define MPU_GYRO_YAW_SIGN           1.0f

#define MPU_SDA_PORT                GPIOA
#define MPU_SDA_PIN                 DL_GPIO_PIN_28
#define MPU_SDA_IOMUX               IOMUX_PINCM3
#define MPU_SCL_PORT                GPIOA
#define MPU_SCL_PIN                 DL_GPIO_PIN_31
#define MPU_SCL_IOMUX               IOMUX_PINCM6

typedef struct {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
} MpuRawData;

typedef struct {
    bool ready;
    uint8_t address;
    float gyroOffsetX;
    float gyroOffsetY;
    float gyroOffsetZ;
    float pitch;
    float roll;
    float yaw;
} MpuState;

static const SoftI2CBus gMpuBus = {
    MPU_SDA_PORT, MPU_SDA_PIN, MPU_SDA_IOMUX,
    MPU_SCL_PORT, MPU_SCL_PIN, MPU_SCL_IOMUX
};

static MpuState gMpuState;

extern void delay_ms(uint32_t ms);

static float mpu_absf(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float mpu_sqrtf(float value)
{
    float guess;
    uint8_t i;

    if (value <= 0.0f) {
        return 0.0f;
    }

    guess = (value > 1.0f) ? value : 1.0f;

    for (i = 0U; i < 6U; i++) {
        guess = 0.5f * (guess + value / guess);
    }

    return guess;
}

static float mpu_atan2f(float y, float x)
{
    float absY = mpu_absf(y) + 0.000001f;
    float angle;
    float ratio;

    if (x >= 0.0f) {
        ratio = (x - absY) / (x + absY);
        angle = (MPU_PI * 0.25f) - (MPU_PI * 0.25f * ratio);
    } else {
        ratio = (x + absY) / (absY - x);
        angle = (MPU_PI * 0.75f) - (MPU_PI * 0.25f * ratio);
    }

    return (y < 0.0f) ? -angle : angle;
}

static bool mpu_read_raw(MpuRawData *raw)
{
    uint8_t frame[14];
    bool ok;

    ok = SoftI2C_ReadRegs(&gMpuBus, gMpuState.address,
        MPU_REG_ACCEL_XOUT_H, frame, 14U);

    if (!ok) {
        return false;
    }

    raw->accelX = (int16_t) (((uint16_t) frame[0] << 8U) | frame[1]);
    raw->accelY = (int16_t) (((uint16_t) frame[2] << 8U) | frame[3]);
    raw->accelZ = (int16_t) (((uint16_t) frame[4] << 8U) | frame[5]);
    raw->gyroX = (int16_t) (((uint16_t) frame[8] << 8U) | frame[9]);
    raw->gyroY = (int16_t) (((uint16_t) frame[10] << 8U) | frame[11]);
    raw->gyroZ = (int16_t) (((uint16_t) frame[12] << 8U) | frame[13]);

    return true;
}

static void mpu_seed_attitude_from_accel(const MpuRawData *raw)
{
    float ax = (float) raw->accelX / 16384.0f;
    float ay = (float) raw->accelY / 16384.0f;
    float az = (float) raw->accelZ / 16384.0f;

    gMpuState.pitch = MPU_DEG_PER_RAD *
        mpu_atan2f(-ax, mpu_sqrtf((ay * ay) + (az * az)));
    gMpuState.roll = MPU_DEG_PER_RAD * mpu_atan2f(ay, az);
    gMpuState.yaw = 0.0f;
}

bool Mpu6050_Init(void)
{
    uint8_t whoAmI = 0U;
    uint8_t addrCandidates[2] = {MPU_I2C_ADDR0, MPU_I2C_ADDR1};
    uint8_t i;
    uint16_t sample;
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;
    MpuRawData raw;

    SoftI2C_Init(&gMpuBus);
    gMpuState.ready = false;

    for (i = 0U; i < 2U; i++) {
        if (SoftI2C_ReadRegs(&gMpuBus, addrCandidates[i],
                MPU_REG_WHO_AM_I, &whoAmI, 1U) && (whoAmI == 0x68U)) {
            gMpuState.address = addrCandidates[i];
            break;
        }
    }

    if (whoAmI != 0x68U) {
        return false;
    }

    if (!SoftI2C_WriteReg8(&gMpuBus, gMpuState.address, MPU_REG_PWR_MGMT_1, 0x00U)) {
        return false;
    }

    delay_ms(50U);
    SoftI2C_WriteReg8(&gMpuBus, gMpuState.address, MPU_REG_SMPLRT_DIV, 0x04U);
    SoftI2C_WriteReg8(&gMpuBus, gMpuState.address, MPU_REG_CONFIG, 0x03U);
    SoftI2C_WriteReg8(&gMpuBus, gMpuState.address, MPU_REG_GYRO_CONFIG, 0x00U);
    SoftI2C_WriteReg8(&gMpuBus, gMpuState.address, MPU_REG_ACCEL_CONFIG, 0x00U);
    delay_ms(20U);

    for (sample = 0U; sample < 220U; sample++) {
        if (!mpu_read_raw(&raw)) {
            return false;
        }

        if (sample >= 20U) {
            sumX += (float) raw.gyroX;
            sumY += (float) raw.gyroY;
            sumZ += (float) raw.gyroZ;
        }

        delay_ms(3U);
    }

    gMpuState.gyroOffsetX = sumX / 200.0f;
    gMpuState.gyroOffsetY = sumY / 200.0f;
    gMpuState.gyroOffsetZ = sumZ / 200.0f;

    if (!mpu_read_raw(&raw)) {
        return false;
    }

    mpu_seed_attitude_from_accel(&raw);
    gMpuState.ready = true;

    return true;
}

bool Mpu6050_Update(void)
{
    MpuRawData raw;
    float ax;
    float ay;
    float az;
    float gyroXDeg;
    float gyroYDeg;
    float gyroZDeg;
    float pitchAcc;
    float rollAcc;
    float dt = 0.005f;

    if (!gMpuState.ready) {
        return false;
    }

    if (!mpu_read_raw(&raw)) {
        return false;
    }

    ax = (float) raw.accelX / 16384.0f;
    ay = (float) raw.accelY / 16384.0f;
    az = (float) raw.accelZ / 16384.0f;

    gyroXDeg = MPU_GYRO_PITCH_SIGN *
        (((float) raw.gyroX - gMpuState.gyroOffsetX) / 131.0f);
    gyroYDeg = MPU_GYRO_ROLL_SIGN *
        (((float) raw.gyroY - gMpuState.gyroOffsetY) / 131.0f);
    gyroZDeg = MPU_GYRO_YAW_SIGN *
        (((float) raw.gyroZ - gMpuState.gyroOffsetZ) / 131.0f);

    pitchAcc = MPU_DEG_PER_RAD *
        mpu_atan2f(-ax, mpu_sqrtf((ay * ay) + (az * az)));
    rollAcc = MPU_DEG_PER_RAD * mpu_atan2f(ay, az);

    gMpuState.pitch = (MPU_ATTITUDE_ALPHA * (gMpuState.pitch + gyroXDeg * dt)) +
                      ((1.0f - MPU_ATTITUDE_ALPHA) * pitchAcc);
    gMpuState.roll = (MPU_ATTITUDE_ALPHA * (gMpuState.roll + gyroYDeg * dt)) +
                     ((1.0f - MPU_ATTITUDE_ALPHA) * rollAcc);
    gMpuState.yaw += gyroZDeg * dt;

    return true;
}

void Mpu6050_GetData(Mpu6050Data *data)
{
    data->ready = gMpuState.ready;
    data->pitch = gMpuState.pitch;
    data->roll = gMpuState.roll;
    data->yaw = gMpuState.yaw;
}
