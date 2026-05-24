#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>

typedef struct {
    bool ready;
    float pitch;
    float roll;
    float yaw;
} Mpu6050Data;

bool Mpu6050_Init(void);
bool Mpu6050_Update(void);
void Mpu6050_GetData(Mpu6050Data *data);

#endif
