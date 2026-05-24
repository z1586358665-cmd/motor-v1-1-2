#include "oled.h"

#include "hardware/soft_i2c/soft_i2c.h"

#define OLED_I2C_ADDR   0x3CU

#define OLED_SDA_PORT   GPIOA
#define OLED_SDA_PIN    DL_GPIO_PIN_0
#define OLED_SDA_IOMUX  IOMUX_PINCM1
#define OLED_SCL_PORT   GPIOA
#define OLED_SCL_PIN    DL_GPIO_PIN_1
#define OLED_SCL_IOMUX  IOMUX_PINCM2

static const SoftI2CBus gOledBus = {
    OLED_SDA_PORT, OLED_SDA_PIN, OLED_SDA_IOMUX,
    OLED_SCL_PORT, OLED_SCL_PIN, OLED_SCL_IOMUX
};

static uint8_t gOledPages[8][128];

static const uint8_t gFontDigits[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x62, 0x51, 0x49, 0x49, 0x46},
    {0x22, 0x49, 0x49, 0x49, 0x36},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x2F, 0x49, 0x49, 0x49, 0x31},
    {0x3E, 0x49, 0x49, 0x49, 0x32},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x26, 0x49, 0x49, 0x49, 0x3E}
};

static const uint8_t gFontUpper[26][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x7F, 0x20, 0x18, 0x20, 0x7F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43}
};

static void oled_write_command(uint8_t cmd)
{
    uint8_t packet[2];

    packet[0] = 0x00U;
    packet[1] = cmd;
    SoftI2C_WriteBytes(&gOledBus, OLED_I2C_ADDR, packet, 2U);
}

static void oled_write_data_block(const uint8_t *data, uint8_t length)
{
    uint8_t packet[17];
    uint8_t i;

    packet[0] = 0x40U;

    for (i = 0U; i < length; i++) {
        packet[i + 1U] = data[i];
    }

    SoftI2C_WriteBytes(&gOledBus, OLED_I2C_ADDR, packet, (uint8_t) (length + 1U));
}

static const uint8_t *oled_lookup_glyph(char c)
{
    static const uint8_t blank[5] = {0, 0, 0, 0, 0};
    static const uint8_t colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t dot[5] = {0x00, 0x40, 0x60, 0x00, 0x00};
    static const uint8_t dash[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
    static const uint8_t plus[5] = {0x08, 0x08, 0x3E, 0x08, 0x08};
    static const uint8_t space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

    if ((c >= '0') && (c <= '9')) {
        return gFontDigits[(uint8_t) (c - '0')];
    }

    if ((c >= 'a') && (c <= 'z')) {
        c = (char) (c - 'a' + 'A');
    }

    if ((c >= 'A') && (c <= 'Z')) {
        return gFontUpper[(uint8_t) (c - 'A')];
    }

    switch (c) {
        case ':':
            return colon;
        case '.':
            return dot;
        case '-':
            return dash;
        case '/':
            return slash;
        case '+':
            return plus;
        case ' ':
            return space;
        default:
            return blank;
    }
}

static void oled_clear_buffer(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0U; page < 8U; page++) {
        for (col = 0U; col < 128U; col++) {
            gOledPages[page][col] = 0x00U;
        }
    }
}

static void oled_draw_text(uint8_t page, uint8_t x, const char *text)
{
    uint8_t col = x;
    uint8_t i;
    const uint8_t *glyph;

    while ((*text != '\0') && (page < 8U) && (col < 123U)) {
        glyph = oled_lookup_glyph(*text++);

        for (i = 0U; (i < 5U) && (col < 128U); i++) {
            gOledPages[page][col++] = glyph[i];
        }

        if (col < 128U) {
            gOledPages[page][col++] = 0x00U;
        }
    }
}

static void oled_flush_all(void)
{
    uint8_t page;
    uint8_t block;

    for (page = 0U; page < 8U; page++) {
        oled_write_command((uint8_t) (0xB0U + page));
        oled_write_command(0x00U);
        oled_write_command(0x10U);

        for (block = 0U; block < 8U; block++) {
            oled_write_data_block(&gOledPages[page][block * 16U], 16U);
        }
    }
}

void Oled_Init(void)
{
    SoftI2C_Init(&gOledBus);

    oled_write_command(0xAEU);
    oled_write_command(0xD5U);
    oled_write_command(0x80U);
    oled_write_command(0xA8U);
    oled_write_command(0x3FU);
    oled_write_command(0xD3U);
    oled_write_command(0x00U);
    oled_write_command(0x40U);
    oled_write_command(0x8DU);
    oled_write_command(0x14U);
    oled_write_command(0x20U);
    oled_write_command(0x02U);
    oled_write_command(0xA1U);
    oled_write_command(0xC8U);
    oled_write_command(0xDAU);
    oled_write_command(0x12U);
    oled_write_command(0x81U);
    oled_write_command(0xCFU);
    oled_write_command(0xD9U);
    oled_write_command(0xF1U);
    oled_write_command(0xDBU);
    oled_write_command(0x40U);
    oled_write_command(0xA4U);
    oled_write_command(0xA6U);
    oled_write_command(0xAFU);

    oled_clear_buffer();
    oled_flush_all();
}

void Oled_ShowLines(const char *line0, const char *line1,
    const char *line2, const char *line3)
{
    oled_clear_buffer();
    oled_draw_text(0U, 0U, line0);
    oled_draw_text(2U, 0U, line1);
    oled_draw_text(4U, 0U, line2);
    oled_draw_text(6U, 0U, line3);
    oled_flush_all();
}
