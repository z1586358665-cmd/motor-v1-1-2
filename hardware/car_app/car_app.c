#include "car_app.h"

#include <stdbool.h>
#include <stdint.h>

#include "hardware/motor/motor.h"
#include "hardware/track_exclude/track.h"
#include "hardware/oled/oled.h"
#include "hardware/mpu6050/mpu6050.h"

#define APP_WAIT_START_MS                 3000U
#define APP_POINT_STOP_MS                 500U
#define APP_DISPLAY_UPDATE_MS             150U

#define APP_STRAIGHT_BASE_SPEED           920
#define APP_STRAIGHT_KP                   18
#define APP_STRAIGHT_MAX_CORRECTION       300
#define APP_STRAIGHT_MIN_MS               550U
#define APP_STRAIGHT_BLANK_CONFIRM_MS     60U
#define APP_STRAIGHT_LINE_CONFIRM_MS      45U
#define APP_STRAIGHT_TIMEOUT_MS           6000U

#define APP_ARC_EXIT_YAW_DEG              165.0f
#define APP_ARC_LOST_CONFIRM_MS           35U
#define APP_ARC_TIMEOUT_MS                5500U

#define APP_LAP_TARGET                    4U

typedef enum {
    APP_STATE_WAIT_START = 0,
    APP_STATE_SEG_AB,
    APP_STATE_STOP_B,
    APP_STATE_SEG_BC,
    APP_STATE_STOP_C,
    APP_STATE_SEG_CD,
    APP_STATE_STOP_D,
    APP_STATE_SEG_DA,
    APP_STATE_STOP_A,
    APP_STATE_FINISHED,
    APP_STATE_ERROR
} AppState;

static AppState gAppState;
static uint32_t gStateTicks;
static uint32_t gDisplayTicks;
static uint8_t gCompletedLaps;
static uint8_t gStableLineTicks;
static uint8_t gStableBlankTicks;
static uint8_t gArcLostTicks;
static bool gStraightSawBlank;
static float gStraightTargetYaw;
static float gArcStartYaw;

static float app_absf(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float app_normalize_angle(float angleDeg)
{
    while (angleDeg > 180.0f) {
        angleDeg -= 360.0f;
    }

    while (angleDeg < -180.0f) {
        angleDeg += 360.0f;
    }

    return angleDeg;
}

static uint32_t app_ms_to_ticks(uint32_t timeMs)
{
    return (timeMs + CAR_APP_LOOP_INTERVAL_MS - 1U) / CAR_APP_LOOP_INTERVAL_MS;
}

static void app_apply_tank_speed(int16_t leftSpeed, int16_t rightSpeed)
{
    if (leftSpeed > MOTOR_SPEED_MAX) {
        leftSpeed = MOTOR_SPEED_MAX;
    } else if (leftSpeed < -MOTOR_SPEED_MAX) {
        leftSpeed = -MOTOR_SPEED_MAX;
    }

    if (rightSpeed > MOTOR_SPEED_MAX) {
        rightSpeed = MOTOR_SPEED_MAX;
    } else if (rightSpeed < -MOTOR_SPEED_MAX) {
        rightSpeed = -MOTOR_SPEED_MAX;
    }

    if (leftSpeed >= 0) {
        Motor_LeftSide(MOTOR_CW, (uint16_t) leftSpeed);
    } else {
        Motor_LeftSide(MOTOR_CCW, (uint16_t) (-leftSpeed));
    }

    if (rightSpeed >= 0) {
        Motor_RightSide(MOTOR_CW, (uint16_t) rightSpeed);
    } else {
        Motor_RightSide(MOTOR_CCW, (uint16_t) (-rightSpeed));
    }
}

static void app_run_straight(float yaw)
{
    float headingError = app_normalize_angle(yaw - gStraightTargetYaw);
    int16_t correction = (int16_t) (headingError * (float) APP_STRAIGHT_KP);
    int16_t leftSpeed;
    int16_t rightSpeed;

    if (correction > APP_STRAIGHT_MAX_CORRECTION) {
        correction = APP_STRAIGHT_MAX_CORRECTION;
    } else if (correction < -APP_STRAIGHT_MAX_CORRECTION) {
        correction = -APP_STRAIGHT_MAX_CORRECTION;
    }

    leftSpeed = (int16_t) (APP_STRAIGHT_BASE_SPEED + correction);
    rightSpeed = (int16_t) (APP_STRAIGHT_BASE_SPEED - correction);
    app_apply_tank_speed(leftSpeed, rightSpeed);
}

static int16_t app_float_to_fixed1(float value)
{
    if (value >= 0.0f) {
        return (int16_t) (value * 10.0f + 0.5f);
    }

    return (int16_t) (value * 10.0f - 0.5f);
}

static void app_format_fixed1(int16_t value10, char *out)
{
    uint16_t absValue;
    uint16_t intPart;
    uint8_t digits[5];
    uint8_t digitCount = 0U;
    uint8_t i;

    if (value10 < 0) {
        *out++ = '-';
        absValue = (uint16_t) (-value10);
    } else {
        *out++ = '+';
        absValue = (uint16_t) value10;
    }

    intPart = absValue / 10U;

    do {
        digits[digitCount++] = (uint8_t) ('0' + (intPart % 10U));
        intPart /= 10U;
    } while ((intPart > 0U) && (digitCount < 5U));

    for (i = digitCount; i > 0U; i--) {
        *out++ = (char) digits[i - 1U];
    }

    *out++ = '.';
    *out++ = (char) ('0' + (absValue % 10U));
    *out = '\0';
}

static void app_build_angle_line(char prefix, float value, char *out)
{
    char valueText[12];
    uint8_t idx = 0U;
    uint8_t copyIdx = 0U;

    app_format_fixed1(app_float_to_fixed1(value), valueText);

    out[idx++] = prefix;
    out[idx++] = ':';

    while ((valueText[copyIdx] != '\0') && (idx < 19U)) {
        out[idx++] = valueText[copyIdx++];
    }

    out[idx] = '\0';
}

static void app_build_status_line(char *out)
{
    uint8_t idx = 0U;
    char segmentA = 'A';
    char segmentB = 'B';

    switch (gAppState) {
        case APP_STATE_SEG_BC:
        case APP_STATE_STOP_B:
            segmentA = 'B';
            segmentB = 'C';
            break;
        case APP_STATE_SEG_CD:
        case APP_STATE_STOP_C:
            segmentA = 'C';
            segmentB = 'D';
            break;
        case APP_STATE_SEG_DA:
        case APP_STATE_STOP_D:
        case APP_STATE_STOP_A:
            segmentA = 'D';
            segmentB = 'A';
            break;
        default:
            segmentA = 'A';
            segmentB = 'B';
            break;
    }

    out[idx++] = 'L';
    out[idx++] = (char) ('0' + gCompletedLaps);
    out[idx++] = '/';
    out[idx++] = (char) ('0' + APP_LAP_TARGET);
    out[idx++] = ' ';
    out[idx++] = segmentA;
    out[idx++] = segmentB;
    out[idx] = '\0';
}

static void app_update_display(const Mpu6050Data *imu)
{
    char line0[20];
    char line1[20];
    char line2[20];
    char line3[20];

    if (!imu->ready) {
        Oled_ShowLines("MPU", "ERROR", "", "");
        return;
    }

    if (gAppState == APP_STATE_WAIT_START) {
        Oled_ShowLines("WAITING", "", "", "");
        return;
    }

    if (gAppState == APP_STATE_FINISHED) {
        Oled_ShowLines("DONE", "4 LAPS", "", "");
        return;
    }

    if (gAppState == APP_STATE_ERROR) {
        Oled_ShowLines("RUN", "ERROR", "", "");
        return;
    }

    app_build_angle_line('P', imu->pitch, line0);
    app_build_angle_line('R', imu->roll, line1);
    app_build_angle_line('Y', app_normalize_angle(imu->yaw), line2);
    app_build_status_line(line3);
    Oled_ShowLines(line0, line1, line2, line3);
}

static void app_enter_state(AppState newState, float yaw)
{
    gAppState = newState;
    gStateTicks = 0U;
    gStableLineTicks = 0U;
    gStableBlankTicks = 0U;
    gArcLostTicks = 0U;
    gStraightSawBlank = false;

    switch (newState) {
        case APP_STATE_WAIT_START:
        case APP_STATE_STOP_B:
        case APP_STATE_STOP_C:
        case APP_STATE_STOP_D:
        case APP_STATE_STOP_A:
        case APP_STATE_FINISHED:
        case APP_STATE_ERROR:
            Motor_Stop();
            break;

        case APP_STATE_SEG_AB:
        case APP_STATE_SEG_CD:
            gStraightTargetYaw = yaw;
            break;

        case APP_STATE_SEG_BC:
        case APP_STATE_SEG_DA:
            Track_Init();
            gArcStartYaw = yaw;
            break;

        default:
            break;
    }
}

void CarApp_Init(void)
{
    Mpu6050Data imu;

    Oled_Init();
    Oled_ShowLines("MPU", "CAL", "", "");

    if (!Mpu6050_Init()) {
        app_enter_state(APP_STATE_ERROR, 0.0f);
        Oled_ShowLines("MPU", "ERROR", "", "");
        return;
    }

    Mpu6050_GetData(&imu);
    gCompletedLaps = 0U;
    gDisplayTicks = 0U;
    app_enter_state(APP_STATE_WAIT_START, imu.yaw);
    app_update_display(&imu);
}

void CarApp_Step(void)
{
    Mpu6050Data imu;
    uint8_t mask;
    bool lineDetected;
    float arcDeltaYaw;

    if (!Mpu6050_Update()) {
        app_enter_state(APP_STATE_ERROR, 0.0f);
    }

    Mpu6050_GetData(&imu);
    mask = Track_Read_State();
    lineDetected = (mask != 0U);

    switch (gAppState) {
        case APP_STATE_WAIT_START:
            if (gStateTicks >= app_ms_to_ticks(APP_WAIT_START_MS)) {
                app_enter_state(APP_STATE_SEG_AB, imu.yaw);
            }
            break;

        case APP_STATE_SEG_AB:
        case APP_STATE_SEG_CD:
            app_run_straight(imu.yaw);

            if (!gStraightSawBlank) {
                if (!lineDetected) {
                    gStableBlankTicks++;
                    if (gStableBlankTicks >= app_ms_to_ticks(APP_STRAIGHT_BLANK_CONFIRM_MS)) {
                        gStraightSawBlank = true;
                    }
                } else {
                    gStableBlankTicks = 0U;
                }
            } else if (gStateTicks >= app_ms_to_ticks(APP_STRAIGHT_MIN_MS)) {
                if (lineDetected) {
                    gStableLineTicks++;
                    if (gStableLineTicks >= app_ms_to_ticks(APP_STRAIGHT_LINE_CONFIRM_MS)) {
                        if (gAppState == APP_STATE_SEG_AB) {
                            app_enter_state(APP_STATE_STOP_B, imu.yaw);
                        } else {
                            app_enter_state(APP_STATE_STOP_D, imu.yaw);
                        }
                    }
                } else {
                    gStableLineTicks = 0U;
                }
            }

            if (gStateTicks >= app_ms_to_ticks(APP_STRAIGHT_TIMEOUT_MS)) {
                app_enter_state(APP_STATE_ERROR, imu.yaw);
            }
            break;

        case APP_STATE_STOP_B:
            if (gStateTicks >= app_ms_to_ticks(APP_POINT_STOP_MS)) {
                app_enter_state(APP_STATE_SEG_BC, imu.yaw);
            }
            break;

        case APP_STATE_SEG_BC:
        case APP_STATE_SEG_DA:
            arcDeltaYaw = app_absf(app_normalize_angle(imu.yaw - gArcStartYaw));
            Track_ControlStep();

            if ((arcDeltaYaw >= APP_ARC_EXIT_YAW_DEG) && !lineDetected) {
                gArcLostTicks++;
                if (gArcLostTicks >= app_ms_to_ticks(APP_ARC_LOST_CONFIRM_MS)) {
                    if (gAppState == APP_STATE_SEG_BC) {
                        app_enter_state(APP_STATE_STOP_C, imu.yaw);
                    } else {
                        gCompletedLaps++;
                        app_enter_state(APP_STATE_STOP_A, imu.yaw);
                    }
                }
            } else {
                gArcLostTicks = 0U;
            }

            if (gStateTicks >= app_ms_to_ticks(APP_ARC_TIMEOUT_MS)) {
                app_enter_state(APP_STATE_ERROR, imu.yaw);
            }
            break;

        case APP_STATE_STOP_C:
            if (gStateTicks >= app_ms_to_ticks(APP_POINT_STOP_MS)) {
                app_enter_state(APP_STATE_SEG_CD, imu.yaw);
            }
            break;

        case APP_STATE_STOP_D:
            if (gStateTicks >= app_ms_to_ticks(APP_POINT_STOP_MS)) {
                app_enter_state(APP_STATE_SEG_DA, imu.yaw);
            }
            break;

        case APP_STATE_STOP_A:
            if (gStateTicks >= app_ms_to_ticks(APP_POINT_STOP_MS)) {
                if (gCompletedLaps >= APP_LAP_TARGET) {
                    app_enter_state(APP_STATE_FINISHED, imu.yaw);
                } else {
                    app_enter_state(APP_STATE_SEG_AB, imu.yaw);
                }
            }
            break;

        case APP_STATE_FINISHED:
        case APP_STATE_ERROR:
        default:
            Motor_Stop();
            break;
    }

    gStateTicks++;
    gDisplayTicks++;

    if (gDisplayTicks >= app_ms_to_ticks(APP_DISPLAY_UPDATE_MS)) {
        gDisplayTicks = 0U;
        app_update_display(&imu);
    }
}
