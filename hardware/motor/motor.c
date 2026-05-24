#include "motor.h"
#include "ti_msp_dl_config.h"

/*
 * ============================================================
 * PWM 相关说明：
 *
 * 你现在的 PWM 周期（Timer LOAD）= 4000
 *
 * 这意味着：
 *   speed = 0       -> 占空比 0%   （完全不输出PWM）
 *   speed = 2000    -> 占空比 50%
 *   speed = 4000    -> 占空比 100% （一直高电平）
 *
 * 电机是否能转起来，取决于占空比大小 + 电机供电 + 驱动能力
 * ============================================================
 */
#define PWM_PERIOD  4000

/*
 * ============================================================
 * set_motor_direction()
 *
 * 功能：
 *   设置电机方向，控制 H 桥的 IN1/IN2
 *
 * 原理：
 *   电机驱动芯片一般都有 IN1 和 IN2
 *
 *   CW（正转）: IN1=1, IN2=0
 *   CCW（反转）: IN1=0, IN2=1
 *
 * 注意：
 *   如果你的电机转向和你想的相反，
 *   只需要交换 IN1/IN2 的高低电平即可
 * ============================================================
 */
static void set_motor_direction(uint8_t motor_id, uint8_t dir)
{
    switch(motor_id)
    {
        case MOTOR_A:
            if(dir == MOTOR_CW)
            {
                // 电机A正转：AIN1=1, AIN2=0
                DL_GPIO_setPins(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN);
                DL_GPIO_clearPins(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN);
            }
            else
            {
                // 电机A反转：AIN1=0, AIN2=1
                DL_GPIO_clearPins(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN);
                DL_GPIO_setPins(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN);
            }
            break;

        case MOTOR_B:
            if(dir == MOTOR_CW)
            {
                // 电机B正转：BIN1=1, BIN2=0
                DL_GPIO_setPins(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN);
                DL_GPIO_clearPins(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN);
            }
            else
            {
                // 电机B反转：BIN1=0, BIN2=1
                DL_GPIO_clearPins(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN);
                DL_GPIO_setPins(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN);
            }
            break;

        case MOTOR_C:
            if(dir == MOTOR_CW)
            {
                // 电机C正转：CIN1=1, CIN2=0
                DL_GPIO_setPins(MOTOR_CIN1_PORT, MOTOR_CIN1_PIN);
                DL_GPIO_clearPins(MOTOR_CIN2_PORT, MOTOR_CIN2_PIN);
            }
            else
            {
                // 电机C反转：CIN1=0, CIN2=1
                DL_GPIO_clearPins(MOTOR_CIN1_PORT, MOTOR_CIN1_PIN);
                DL_GPIO_setPins(MOTOR_CIN2_PORT, MOTOR_CIN2_PIN);
            }
            break;

        case MOTOR_D:
            if(dir == MOTOR_CW)
            {
                // 电机D正转：DIN1=1, DIN2=0
                DL_GPIO_setPins(MOTOR_DIN1_PORT, MOTOR_DIN1_PIN);
                DL_GPIO_clearPins(MOTOR_DIN2_PORT, MOTOR_DIN2_PIN);
            }
            else
            {
                // 电机D反转：DIN1=0, DIN2=1
                DL_GPIO_clearPins(MOTOR_DIN1_PORT, MOTOR_DIN1_PIN);
                DL_GPIO_setPins(MOTOR_DIN2_PORT, MOTOR_DIN2_PIN);
            }
            break;

        default:
            // 未知 motor_id，不处理
            break;
    }
}

/*
 * ============================================================
 * set_motor_speed()
 *
 * 功能：
 *   设置 PWM 占空比（通过修改 Timer CCR 比较值实现）
 *
 * 参数：
 *   speed : 0 ~ PWM_PERIOD (0~4000)
 *
 * 原理：
 *   PWM输出由 Timer Compare 寄存器决定
 *   Compare 值越大，占空比越大，电机转得越快
 *
 * 注意：
 *   速度过小（比如 100、200、300）占空比只有 2.5%~7.5%
 *   电机可能启动不了，会出现“慢/抖/不转”
 * ============================================================
 */
static uint16_t motor_apply_pwm_gain(uint8_t motor_id, uint16_t speed)
{
    uint32_t gain = 1000U;
    uint32_t scaled;

    switch (motor_id) {
        case MOTOR_A:
            gain = MOTOR_PWM_GAIN_A;
            break;
        case MOTOR_B:
            gain = MOTOR_PWM_GAIN_B;
            break;
        case MOTOR_C:
            gain = MOTOR_PWM_GAIN_C;
            break;
        case MOTOR_D:
            gain = MOTOR_PWM_GAIN_D;
            break;
        default:
            break;
    }

    scaled = ((uint32_t) speed * gain) / 1000U;
    if (scaled > (uint32_t) PWM_PERIOD) {
        scaled = (uint32_t) PWM_PERIOD;
    }

    return (uint16_t) scaled;
}

static void set_motor_speed(uint8_t motor_id, uint16_t speed)
{
    // 防止 speed 超出 PWM 周期范围
    if(speed > PWM_PERIOD)
    {
        speed = PWM_PERIOD;
    }

    switch(motor_id)
    {
        case MOTOR_A:
            // 电机A：PWM_A_INST 使用 CC1 通道输出PWM
            DL_TimerA_setCaptureCompareValue(PWM_A_INST, speed, DL_TIMER_CC_1_INDEX);
            break;

        case MOTOR_B:
            // 电机B：PWM_B_INST 使用 CC1 通道输出PWM
            DL_TimerG_setCaptureCompareValue(PWM_B_INST, speed, DL_TIMER_CC_1_INDEX);
            break;

        case MOTOR_C:
            // 电机C：PWM_B_INST 使用 CC0 通道输出PWM
            DL_TimerG_setCaptureCompareValue(PWM_B_INST, speed, DL_TIMER_CC_0_INDEX);
            break;

        case MOTOR_D:
            // 电机D：PWM_D_INST 使用 CC1 通道输出PWM
            DL_TimerG_setCaptureCompareValue(PWM_D_INST, speed, DL_TIMER_CC_1_INDEX);
            break;

        default:
            // 未知 motor_id，不处理
            break;
    }
}

/*
 * ============================================================
 * Motor_Init()
 *
 * 功能：
 *   初始化电机驱动：
 *   1. 启动 PWM 定时器计数
 *   2. 将所有电机设置为停止状态
 *
 * 注意：
 *   SYSCFG_DL_init() 会初始化定时器参数和GPIO复用
 *   Motor_Init() 负责启动计数器，使PWM真正输出
 * ============================================================
 */
void Motor_Init(void)
{
    // 启动 PWM 定时器计数器，PWM开始输出
    DL_TimerA_startCounter(PWM_A_INST);
    DL_TimerG_startCounter(PWM_B_INST);
    DL_TimerG_startCounter(PWM_D_INST);

    // 默认停止所有电机
    Motor_Stop();
}

/*
 * ============================================================
 * Motor_Control()
 *
 * 功能：
 *   控制单个电机：方向 + 速度
 *
 * 参数：
 *   motor_id : MOTOR_A / MOTOR_B / MOTOR_C / MOTOR_D
 *   dir      : MOTOR_CW / MOTOR_CCW
 *   speed    : 0~4000 (对应0%~100%占空比)
 *
 * 注意：
 *   speed=0 时，只设置PWM为0，不做刹车
 *   如果你想“刹车”（快速停），需要让 IN1=IN2=1 或 IN1=IN2=0
 *   取决于驱动芯片的刹车模式
 * ============================================================
 */
void Motor_Control(uint8_t motor_id, uint8_t dir, uint16_t speed)
{
    // speed=0 表示停止
    if(speed == 0)
    {
        set_motor_speed(motor_id, 0);
        return;
    }

    // 先设置方向
    set_motor_direction(motor_id, dir);

    // 再设置PWM占空比（按电机微调，尽量四轮直线一致）
    set_motor_speed(motor_id, motor_apply_pwm_gain(motor_id, speed));
}

/*
 * ============================================================
 * Motor_ControlAll()
 *
 * 功能：
 *   同时控制四个电机
 *
 * 用途：
 *   小车前进、后退、转向时需要一次性设置多个电机
 * ============================================================
 */
void Motor_ControlAll(uint8_t dir_a, uint16_t speed_a,
                      uint8_t dir_b, uint16_t speed_b,
                      uint8_t dir_c, uint16_t speed_c,
                      uint8_t dir_d, uint16_t speed_d)
{
    Motor_Control(MOTOR_A, dir_a, speed_a);
    Motor_Control(MOTOR_B, dir_b, speed_b);
    Motor_Control(MOTOR_C, dir_c, speed_c);
    Motor_Control(MOTOR_D, dir_d, speed_d);
}

/*
 * ============================================================
 * Motor_Forward()
 *
 * 功能：
 *   小车四个轮子同方向转动，实现前进
 * ============================================================
 */
void Motor_Forward(uint16_t speed)
{
    Motor_ControlAll(MOTOR_CW, speed,
                     MOTOR_CW, speed,
                     MOTOR_CW, speed,
                     MOTOR_CW, speed);
}

/*
 * ============================================================
 * Motor_Backward()
 *
 * 功能：
 *   小车四个轮子同方向转动，实现后退
 * ============================================================
 */
void Motor_Backward(uint16_t speed)
{
    Motor_ControlAll(MOTOR_CCW, speed,
                     MOTOR_CCW, speed,
                     MOTOR_CCW, speed,
                     MOTOR_CCW, speed);
}

/*
 * ============================================================
 * Motor_TurnLeft()
 *
 * 功能：
 *   小车左转：
 *   左侧轮子反转，右侧轮子正转
 * ============================================================
 */
void Motor_TurnLeft(uint16_t speed)
{
    Motor_ControlAll(MOTOR_CCW, speed,   // 左侧A反转
                     MOTOR_CW, speed,    // 右侧B正转
                     MOTOR_CCW, speed,   // 左侧C反转
                     MOTOR_CW, speed);   // 右侧D正转
}

/*
 * ============================================================
 * Motor_TurnRight()
 *
 * 功能：
 *   小车右转：
 *   左侧轮子正转，右侧轮子反转
 * ============================================================
 */
void Motor_TurnRight(uint16_t speed)
{
    Motor_ControlAll(MOTOR_CW, speed,    // 左侧A正转
                     MOTOR_CCW, speed,   // 右侧B反转
                     MOTOR_CW, speed,    // 左侧C正转
                     MOTOR_CCW, speed);  // 右侧D反转
}

/*
 * ============================================================
 * Motor_Stop()
 *
 * 功能：
 *   停止所有电机
 *
 * 注意：
 *   这里只是PWM=0，属于“滑行停止”
 *   如果你想“刹车停止”，可以把IN1=IN2=1（或0）
 * ============================================================
 */
void Motor_Stop(void)
{
    Motor_ControlAll(MOTOR_CW, 0,
                     MOTOR_CW, 0,
                     MOTOR_CW, 0,
                     MOTOR_CW, 0);
}

/*
 * ============================================================
 * Motor_LeftSide()
 *
 * 功能：
 *   控制左侧两个电机（A+C）
 * ============================================================
 */
void Motor_LeftSide(uint8_t dir, uint16_t speed)
{
    Motor_Control(MOTOR_A, dir, speed);
    Motor_Control(MOTOR_C, dir, speed);
}

/*
 * ============================================================
 * Motor_RightSide()
 *
 * 功能：
 *   控制右侧两个电机（B+D）
 * ============================================================
 */
void Motor_RightSide(uint8_t dir, uint16_t speed)
{
    Motor_Control(MOTOR_B, dir, speed);
    Motor_Control(MOTOR_D, dir, speed);
}

