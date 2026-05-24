#include "soft_i2c.h"

extern void delay_ms(uint32_t ms);

static void soft_i2c_delay_short(void)
{
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static void soft_i2c_scl_output(const SoftI2CBus *bus)
{
    DL_GPIO_initDigitalOutput(bus->sclIomux);
    DL_GPIO_enableOutput(bus->sclPort, bus->sclPin);
}

static void soft_i2c_sda_output(const SoftI2CBus *bus)
{
    DL_GPIO_initDigitalOutput(bus->sdaIomux);
    DL_GPIO_enableOutput(bus->sdaPort, bus->sdaPin);
}

static void soft_i2c_sda_input(const SoftI2CBus *bus)
{
    DL_GPIO_initDigitalInputFeatures(bus->sdaIomux,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
}

static void soft_i2c_set_scl(const SoftI2CBus *bus, bool high)
{
    if (high) {
        DL_GPIO_setPins(bus->sclPort, bus->sclPin);
    } else {
        DL_GPIO_clearPins(bus->sclPort, bus->sclPin);
    }
}

static void soft_i2c_set_sda(const SoftI2CBus *bus, bool high)
{
    if (high) {
        DL_GPIO_setPins(bus->sdaPort, bus->sdaPin);
    } else {
        DL_GPIO_clearPins(bus->sdaPort, bus->sdaPin);
    }
}

static bool soft_i2c_read_sda(const SoftI2CBus *bus)
{
    return (DL_GPIO_readPins(bus->sdaPort, bus->sdaPin) != 0U);
}

static void soft_i2c_start(const SoftI2CBus *bus)
{
    soft_i2c_sda_output(bus);
    soft_i2c_set_sda(bus, true);
    soft_i2c_set_scl(bus, true);
    soft_i2c_delay_short();
    soft_i2c_set_sda(bus, false);
    soft_i2c_delay_short();
    soft_i2c_set_scl(bus, false);
}

static void soft_i2c_stop(const SoftI2CBus *bus)
{
    soft_i2c_sda_output(bus);
    soft_i2c_set_sda(bus, false);
    soft_i2c_delay_short();
    soft_i2c_set_scl(bus, true);
    soft_i2c_delay_short();
    soft_i2c_set_sda(bus, true);
    soft_i2c_delay_short();
}

static bool soft_i2c_write_byte(const SoftI2CBus *bus, uint8_t value)
{
    uint8_t bit;
    bool ack;

    soft_i2c_sda_output(bus);

    for (bit = 0U; bit < 8U; bit++) {
        soft_i2c_set_sda(bus, ((value & 0x80U) != 0U));
        soft_i2c_delay_short();
        soft_i2c_set_scl(bus, true);
        soft_i2c_delay_short();
        soft_i2c_set_scl(bus, false);
        value <<= 1U;
    }

    soft_i2c_sda_input(bus);
    soft_i2c_delay_short();
    soft_i2c_set_scl(bus, true);
    soft_i2c_delay_short();
    ack = !soft_i2c_read_sda(bus);
    soft_i2c_set_scl(bus, false);
    soft_i2c_sda_output(bus);
    soft_i2c_set_sda(bus, true);

    return ack;
}

static uint8_t soft_i2c_read_byte(const SoftI2CBus *bus, bool ack)
{
    uint8_t value = 0U;
    uint8_t bit;

    soft_i2c_sda_input(bus);

    for (bit = 0U; bit < 8U; bit++) {
        value <<= 1U;
        soft_i2c_set_scl(bus, true);
        soft_i2c_delay_short();
        if (soft_i2c_read_sda(bus)) {
            value |= 0x01U;
        }
        soft_i2c_set_scl(bus, false);
        soft_i2c_delay_short();
    }

    soft_i2c_sda_output(bus);
    soft_i2c_set_sda(bus, !ack);
    soft_i2c_delay_short();
    soft_i2c_set_scl(bus, true);
    soft_i2c_delay_short();
    soft_i2c_set_scl(bus, false);
    soft_i2c_set_sda(bus, true);

    return value;
}

void SoftI2C_Init(const SoftI2CBus *bus)
{
    soft_i2c_scl_output(bus);
    soft_i2c_sda_output(bus);
    soft_i2c_set_sda(bus, true);
    soft_i2c_set_scl(bus, true);
    delay_ms(1U);
}

bool SoftI2C_WriteBytes(const SoftI2CBus *bus,
    uint8_t devAddr, const uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    bool ok = true;

    soft_i2c_start(bus);
    ok &= soft_i2c_write_byte(bus, (uint8_t) (devAddr << 1U));

    for (i = 0U; (i < length) && ok; i++) {
        ok &= soft_i2c_write_byte(bus, buffer[i]);
    }

    soft_i2c_stop(bus);

    return ok;
}

bool SoftI2C_WriteReg8(const SoftI2CBus *bus,
    uint8_t devAddr, uint8_t regAddr, uint8_t value)
{
    bool ok = true;

    soft_i2c_start(bus);
    ok &= soft_i2c_write_byte(bus, (uint8_t) (devAddr << 1U));
    ok &= soft_i2c_write_byte(bus, regAddr);
    ok &= soft_i2c_write_byte(bus, value);
    soft_i2c_stop(bus);

    return ok;
}

bool SoftI2C_ReadRegs(const SoftI2CBus *bus,
    uint8_t devAddr, uint8_t regAddr, uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    bool ok = true;

    soft_i2c_start(bus);
    ok &= soft_i2c_write_byte(bus, (uint8_t) (devAddr << 1U));
    ok &= soft_i2c_write_byte(bus, regAddr);
    soft_i2c_start(bus);
    ok &= soft_i2c_write_byte(bus, (uint8_t) ((devAddr << 1U) | 0x01U));

    for (i = 0U; (i < length) && ok; i++) {
        buffer[i] = soft_i2c_read_byte(bus, (i + 1U) < length);
    }

    soft_i2c_stop(bus);

    return ok;
}
