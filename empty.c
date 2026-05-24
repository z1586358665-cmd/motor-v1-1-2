#include "ti_msp_dl_config.h"
#include "hardware/motor/motor.h"
#include "hardware/track_exclude/track.h"
#include "hardware/car_app/car_app.h"

static void delay_us(uint32_t us)
{
    volatile uint32_t i;

    for (i = 0U; i < (us * 8U); i++) {
        __NOP();
    }
}

void delay_ms(uint32_t ms)
{
    uint32_t i;

    for (i = 0U; i < ms; i++) {
        delay_us(1000U);
    }
}

int main(void)
{
    SYSCFG_DL_init();
    Motor_Init();
    Track_Init();
    CarApp_Init();

    while (1) {
        CarApp_Step();
        delay_ms(CAR_APP_LOOP_INTERVAL_MS);
    }
}
