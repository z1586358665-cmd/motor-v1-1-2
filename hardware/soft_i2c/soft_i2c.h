#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

typedef struct {
    GPIO_Regs *sdaPort;
    uint32_t sdaPin;
    uint32_t sdaIomux;
    GPIO_Regs *sclPort;
    uint32_t sclPin;
    uint32_t sclIomux;
} SoftI2CBus;

void SoftI2C_Init(const SoftI2CBus *bus);
bool SoftI2C_WriteBytes(const SoftI2CBus *bus,
    uint8_t devAddr, const uint8_t *buffer, uint8_t length);
bool SoftI2C_WriteReg8(const SoftI2CBus *bus,
    uint8_t devAddr, uint8_t regAddr, uint8_t value);
bool SoftI2C_ReadRegs(const SoftI2CBus *bus,
    uint8_t devAddr, uint8_t regAddr, uint8_t *buffer, uint8_t length);

#endif
