#include "track.h"

#include "hardware/motor/motor.h"
#include "ti_msp_dl_config.h"

/* ============================================================
 * TrackSensor 结构体
 *
 * 用于保存每个循迹传感器对应的 GPIO 端口和引脚号。
 *
 * 这样写的好处：
 * - 以后更换传感器接线只需要修改数组，不用改算法代码
 * - 循迹算法读取传感器状态时可以用循环统一读取
 * ============================================================ */
typedef struct {
    GPIO_Regs *port;   /* GPIO端口寄存器指针，例如 GPIOA / GPIOB */
    uint32_t pin;      /* GPIO引脚掩码，例如 DL_GPIO_PIN_0 */
} TrackSensor;


/* ============================================================
 * 8路循迹传感器映射表
 *
 * gTrackSensors[0] -> XIN1 （最左）
 * gTrackSensors[7] -> XIN8 （最右）
 *
 * 注意：这里顺序非常关键
 * 如果你发现车修正方向反了（偏左却向左修正），
 * 需要检查：
 * 1) 传感器顺序是否接反
 * 2) 权重数组是否正负写反
 * ============================================================ */
static const TrackSensor gTrackSensors[TRACK_SENSOR_COUNT] = {
    {TRACK_XIN1_PORT, TRACK_XIN1_PIN},
    {TRACK_XIN2_PORT, TRACK_XIN2_PIN},
    {TRACK_XIN3_PORT, TRACK_XIN3_PIN},
    {TRACK_XIN4_PORT, TRACK_XIN4_PIN},
    {TRACK_XIN5_PORT, TRACK_XIN5_PIN},
    {TRACK_XIN6_PORT, TRACK_XIN6_PIN},
    {TRACK_XIN7_PORT, TRACK_XIN7_PIN},
    {TRACK_XIN8_PORT, TRACK_XIN8_PIN},
};


/* ============================================================
 * 权重数组 gTrackWeights
 *
 * 作用：
 * 通过加权平均计算小车偏离黑线中心的误差 error
 *
 * 设计思想：
 * - 左边传感器为负权重
 * - 右边传感器为正权重
 * - 中间 OUT4/OUT5 权重较小，表示小车基本居中
 *
 * error > 0：黑线偏右，小车需要向右修正
 * error < 0：黑线偏左，小车需要向左修正
 *
 * 数值越大代表偏差越大，修正越强
 * ============================================================ */
static const int16_t gTrackWeights[TRACK_SENSOR_COUNT] = {
    -2800, -1900, -1100, -350, 350, 1100, 1900, 2800
};


/* ============================================================
 * 优先级权重数组 gTrackPriorityWeights
 *
 * 作用：
 * 用于增强中间传感器（OUT4/OUT5）的影响力，让车尽量以中间对准线。
 *
 * 例如 OUT4/OUT5 权重设置为 6，
 * 表示如果 OUT4/OUT5 检测到黑线，它们的影响更大，
 * 这样小车会更倾向于让线保持在中间。
 *
 * 注意：
 * 这里不是位置权重，而是“参与加权平均的倍数权重”。
 * ============================================================ */
static const uint8_t gTrackPriorityWeights[TRACK_SENSOR_COUNT] = {
    1U, 2U, 3U, 6U, 6U, 3U, 2U, 1U
};


/* ============================================================
 * 全局状态变量
 * ============================================================ */

/* 上一次循环的误差值（用于微分项 deltaError） */
static int16_t gTrackLastError;

/* 上一次有效误差（用于丢线时维持方向） */
static int16_t gTrackLastValidError;

/* 上一次控制输出 control（用于滤波/限速） */
static int16_t gTrackLastControl;

/* 上一次转向方向：
 *  1  表示最近是向右转
 * -1  表示最近是向左转
 *  0  表示尚未确定方向 */
static int8_t gTrackLastTurn;

/* 丢线计数器：mask == 0 时累加，用于去抖确认丢线事件 */
static uint8_t gTrackLostTicks;

/* 上一周期是否检测到线（用于判断“从有到无”的丢线边沿） */
static uint8_t gTrackLastLoopHadLine;

/* 已确认的丢线次数（每次从有线连续丢线确认后 +1，重新见线后允许下次再计） */
static uint8_t gTrackLossEventCount;

/* 当前这一段连续丢线内是否已为该段加过丢线计数（防止同一段丢线重复计数） */
static uint8_t gTrackLossCountedThisSession;


/* ============================================================
 * track_abs16()
 * 取 int16 的绝对值
 * ============================================================ */
static int16_t track_abs16(int16_t value)
{
    return (value < 0) ? (int16_t) (-value) : value;
}


/* ============================================================
 * track_clamp16()
 * 限幅函数：把 value 限制在 [minValue, maxValue]
 *
 * 常用于：
 * - 防止 control 超过最大纠偏能力
 * - 防止速度超出最大PWM范围
 * ============================================================ */
static int16_t track_clamp16(int16_t value, int16_t minValue, int16_t maxValue)
{
    if (value < minValue) {
        return minValue;
    }

    if (value > maxValue) {
        return maxValue;
    }

    return value;
}


/* ============================================================
 * track_blend16()
 * 一阶低通滤波（加权平均滤波）
 *
 * 用途：
 * - 对 error 进行平滑滤波，减少噪声导致的左右摆动
 * - 对 control 进行平滑滤波，减少顿挫感
 *
 * 公式：
 * blended = (previous*(totalWeight-currentWeight) + current*currentWeight)
 *           / totalWeight
 *
 * 举例：
 * currentWeight=3 totalWeight=4
 * -> previous占比=1/4，current占比=3/4（响应快）
 *
 * currentWeight=1 totalWeight=4
 * -> previous占比=3/4，current占比=1/4（更平滑）
 * ============================================================ */
static int16_t track_blend16(int16_t previous, int16_t current,
    uint8_t currentWeight, uint8_t totalWeight)
{
    int32_t blendedValue = (int32_t) previous *
                               (int32_t) (totalWeight - currentWeight) +
                           (int32_t) current * (int32_t) currentWeight;

    return (int16_t) (blendedValue / (int32_t) totalWeight);
}


/* ============================================================
 * track_limit_step()
 * 控制量限速函数（防止 control 一次跳变过大）
 *
 * 用途：
 * - 防止在弯道或S弯时突然猛打方向导致顿挫
 * - 防止塑料布反光造成 mask 抖动，从而导致 control 突然变大
 *
 * maxStep 表示本次允许变化的最大幅度
 * ============================================================ */
static int16_t track_limit_step(int16_t target, int16_t current, int16_t maxStep)
{
    if (target > (int16_t) (current + maxStep)) {
        return (int16_t) (current + maxStep);
    }

    if (target < (int16_t) (current - maxStep)) {
        return (int16_t) (current - maxStep);
    }

    return target;
}


/* ============================================================
 * track_read_raw_mask()
 *
 * 读取8路循迹传感器的原始状态，组合成 mask
 *
 * bit0 -> XIN1
 * bit7 -> XIN8
 *
 * 你的传感器逻辑为：
 * 黑线输出 = 1
 * 白底输出 = 0
 *
 * mask 的意义：
 * 例如 mask = 0b00011000
 * 表示 OUT4 和 OUT5 检测到黑线，小车基本居中
 * ============================================================ */
static uint8_t track_read_raw_mask(void)
{
    uint8_t mask = 0U;
    uint8_t i;

    for (i = 0U; i < TRACK_SENSOR_COUNT; i++) {
        if (DL_GPIO_readPins(gTrackSensors[i].port, gTrackSensors[i].pin) != 0U) {
            mask |= (uint8_t) (1U << i);
        }
    }

    return mask;
}


/* ============================================================
 * track_get_edge_bias()
 *
 * 功能：
 * 判断小车当前是否偏向轨道边缘，并给出一个偏向提示值：
 *
 * 返回值：
 *  -1 : 左侧传感器触发明显，说明线在左侧，车偏右，需要左转
 *   1 : 右侧传感器触发明显，说明线在右侧，车偏左，需要右转
 *   0 : 中间区域检测到线，认为无明显偏边趋势
 *
 * 为什么需要这个？
 * 因为当车偏得很厉害时，普通加权误差可能不够强，
 * edgeBias 可以让系统进入“硬转弯模式”（hard turn）。
 * ============================================================ */
static int8_t track_get_edge_bias(uint8_t mask)
{
    uint8_t leftActive = mask & 0x07U;   /* OUT1~OUT3 */
    uint8_t rightActive = mask & 0xE0U;  /* OUT6~OUT8 */

    /* 如果中间 OUT4/OUT5 有线，则认为不偏边 */
    if ((mask & 0x18U) != 0U) {
        return 0;
    }

    /* 左侧有线，右侧无 -> 说明线偏左 -> 应该左转 */
    if ((leftActive != 0U) && (rightActive == 0U)) {
        return -1;
    }

    /* 右侧有线，左侧无 -> 说明线偏右 -> 应该右转 */
    if ((rightActive != 0U) && (leftActive == 0U)) {
        return 1;
    }

    /* OUT1/OUT2 检测到线 -> 强烈偏左 */
    if ((mask & 0x03U) != 0U) {
        return -1;
    }

    /* OUT7/OUT8 检测到线 -> 强烈偏右 */
    if ((mask & 0xC0U) != 0U) {
        return 1;
    }

    return 0;
}


/* ============================================================
 * track_calc_error()
 *
 * 功能：
 * 根据 mask 计算偏差 error（加权平均）
 *
 * 这里的算法比普通加权平均更高级：
 * - 引入 priority 权重，让 OUT4/OUT5 更重要
 * - 当中间检测到线时，降低两侧权重的影响
 *
 * activeCount:
 * 输出参数，表示当前有多少个传感器检测到黑线
 *
 * 结果：
 * error 越大表示偏得越厉害
 * ============================================================ */
static int16_t track_calc_error(uint8_t mask, uint8_t *activeCount)
{
    int32_t weightedSum = 0;
    uint16_t prioritySum = 0U;
    uint8_t count = 0U;

    /* centerActive=1 表示 OUT4/OUT5 至少一个检测到线 */
    uint8_t centerActive = ((mask & 0x18U) != 0U) ? 1U : 0U;
    uint8_t i;

    for (i = 0U; i < TRACK_SENSOR_COUNT; i++) {
        if ((mask & (uint8_t) (1U << i)) != 0U) {

            /* 取该传感器的优先级权重 */
            uint8_t priority = gTrackPriorityWeights[i];

            /*
             * 如果中间检测到线，说明车大概率还在轨道附近，
             * 这时候 OUT1/OUT8 可能是噪声或者边缘反光，
             * 因此降低边缘优先级，避免误修正。
             */
            if (centerActive != 0U) {
                if ((i == 0U) || (i == 7U)) {
                    priority = 1U;
                } else if ((i == 1U) || (i == 6U)) {
                    priority = 2U;
                }
            }

            /* priority为0表示该点忽略（本代码没有0，但预留） */
            if (priority != 0U) {
                weightedSum += (int32_t) gTrackWeights[i] * (int32_t) priority;
                prioritySum += priority;
            }

            count++;
        }
    }

    *activeCount = count;

    /*
     * prioritySum==0 表示没有任何有效数据
     * 返回上一次有效误差，避免瞬间跳变
     */
    if (prioritySum == 0U) {
        return gTrackLastValidError;
    }

    return (int16_t) (weightedSum / (int32_t) prioritySum);
}


/* ============================================================
 * track_calc_base_speed()
 *
 * 功能：
 * 根据当前状态动态计算 baseSpeed（基础速度）
 *
 * 思想：
 * - error 越大，说明偏离越严重，需要减速以提高稳定性
 * - 中间没检测到线，也减速（说明车偏了）
 * - 边缘传感器触发，进一步减速（快跑出线）
 * - activeCount多（弯道/宽线），减速
 *
 * 最终 baseSpeed 会被 clamp 在 [CURVE_SPEED, BASE_SPEED]
 * ============================================================ */
static int16_t track_calc_base_speed(uint8_t mask, uint8_t activeCount, int16_t error)
{
    int16_t baseSpeed = (int16_t) TRACK_BASE_SPEED;

    /* error 越大，减速越多 */
    baseSpeed -= (int16_t) (track_abs16(error) / 5);

    /* OUT4/OUT5都没检测到线 -> 说明偏离中心 -> 再减速 */
    if ((mask & 0x18U) == 0U) {
        baseSpeed -= 100;
    }

    /* OUT1 或 OUT8 触发 -> 快跑出线 -> 减速 */
    if ((mask & 0x81U) != 0U) {
        baseSpeed -= 120;
    }

    /* activeCount大说明弯道或宽线区域 -> 减速 */
    if (activeCount >= TRACK_CURVE_ACTIVE_THRESHOLD) {
        baseSpeed -= 80;
    }

    /* 限制 baseSpeed 范围 */
    return track_clamp16(baseSpeed,
        (int16_t) TRACK_CURVE_SPEED,
        (int16_t) TRACK_BASE_SPEED);
}


/* ============================================================
 * track_apply_speed()
 *
 * 功能：
 * 根据 leftSpeed / rightSpeed 输出到电机
 *
 * 支持负数：
 * - leftSpeed >= 0 -> 左轮正转
 * - leftSpeed < 0  -> 左轮反转
 *
 * 同理右轮
 *
 * 注意：
 * 速度会被 clamp 到 [-TRACK_MAX_SPEED, TRACK_MAX_SPEED]
 * ============================================================ */
static void track_apply_speed(int16_t leftSpeed, int16_t rightSpeed)
{
    leftSpeed = track_clamp16(leftSpeed,
        -(int16_t) TRACK_MAX_SPEED,
        (int16_t) TRACK_MAX_SPEED);

    rightSpeed = track_clamp16(rightSpeed,
        -(int16_t) TRACK_MAX_SPEED,
        (int16_t) TRACK_MAX_SPEED);

    if (leftSpeed >= 0) {
        Motor_LeftSide(MOTOR_CW, (uint16_t) leftSpeed);
    } else {
        Motor_LeftSide(MOTOR_CCW, (uint16_t) track_abs16(leftSpeed));
    }

    if (rightSpeed >= 0) {
        Motor_RightSide(MOTOR_CW, (uint16_t) rightSpeed);
    } else {
        Motor_RightSide(MOTOR_CCW, (uint16_t) track_abs16(rightSpeed));
    }
}


/* ============================================================
 * Track_Init()
 *
 * 初始化循迹模块状态变量
 * ============================================================ */
void Track_Init(void)
{
    gTrackLastError = 0;
    gTrackLastValidError = 0;
    gTrackLastControl = 0;
    gTrackLastTurn = 0;
    gTrackLostTicks = 0U;
    gTrackLastLoopHadLine = 0U;
    gTrackLossEventCount = 0U;
    gTrackLossCountedThisSession = 0U;

    /* 初始化时停止电机，防止突然启动 */
    Motor_Stop();
}


/* ============================================================
 * Track_Read_State()
 *
 * 三次采样多数表决（抗抖动）
 *
 * majority = (A&B) | (A&C) | (B&C)
 *
 * 作用：
 * - 防止塑料布反光导致瞬间 0/1 跳变
 * - 提高传感器输出稳定性
 * ============================================================ */
uint8_t Track_Read_State(void)
{
    uint8_t sampleA = track_read_raw_mask();
    uint8_t sampleB = track_read_raw_mask();
    uint8_t sampleC = track_read_raw_mask();

    return (uint8_t) ((sampleA & sampleB) | (sampleA & sampleC) | (sampleB & sampleC));
}


/* ============================================================
 * Track_GetLastError()
 *
 * 返回上一次滤波后的误差值（用于调试显示）
 * ============================================================ */
int16_t Track_GetLastError(void)
{
    return gTrackLastError;
}


/* ============================================================
 * Track_ControlStep()
 *
 * 循迹控制主函数（核心逻辑）
 *
 * 建议每 5ms 调用一次
 *
 * 主要流程：
 * 1. 读取 mask
 * 2. 丢线处理（mask==0）
 * 3. 计算误差 measuredError
 * 4. error滤波（filteredError）
 * 5. 根据误差和mask计算基础速度 baseSpeed
 * 6. PD 计算目标控制量 targetControl
 * 7. control滤波 + 限速变化
 * 8. 输出左右轮差速
 * ============================================================ */
void Track_ControlStep(void)
{
    uint8_t mask = Track_Read_State();      /* 当前8路传感器状态 */
    uint8_t prevHadLine = gTrackLastLoopHadLine;
    uint8_t activeCount = 0U;               /* 当前检测到黑线的传感器数量 */
    int8_t edgeBias;                        /* 边缘偏向提示（硬转弯用） */

    int16_t measuredError;                  /* 原始误差（未滤波） */
    int16_t filteredError;                  /* 滤波后的误差（更稳定） */
    int16_t baseSpeed;                      /* 当前基础速度 */

    int16_t deltaError;                     /* 微分项：error变化量 */
    int16_t targetControl;                  /* PD计算得到的目标control */
    int16_t control;                        /* 实际输出control（经过滤波/限速） */

    int16_t stepLimit;                      /* control变化限速值 */

    int16_t leftSpeed;                      /* 左轮速度 */
    int16_t rightSpeed;                     /* 右轮速度 */


    /* ============================================================
     * 1. 丢线处理
     *
     * mask==0 表示所有传感器都没检测到黑线
     * 可能原因：
     * - 车完全跑出线
     * - 塑料布反光导致瞬间误判
     *
     * 策略：
     * - 丢线后左右轮同速直线前进，不再原地旋转循迹搜线
     * - 从“上一周期有线”经过去抖确认后计为一次丢线事件；
     *   第二次丢线事件后停车（需 Track_Init 后才会重新计数）
     * ============================================================ */
    if (mask == 0U) {
        gTrackLostTicks++;

        if ((gTrackLostTicks >= TRACK_LOST_EVENT_CONFIRM_TICKS) &&
            (gTrackLossCountedThisSession == 0U) &&
            (prevHadLine != 0U)) {
            gTrackLossEventCount++;
            gTrackLossCountedThisSession = 1U;
        }

        if (gTrackLossEventCount >= 2U) {
            Motor_Stop();
            gTrackLastLoopHadLine = 0U;
            return;
        }

        track_apply_speed((int16_t) TRACK_LOST_STRAIGHT_SPEED,
            (int16_t) TRACK_LOST_STRAIGHT_SPEED);
        gTrackLastLoopHadLine = 0U;
        return;
    }


    /* ============================================================
     * 2. 正常循迹：计算误差 measuredError
     * ============================================================ */
    measuredError = track_calc_error(mask, &activeCount);

    /* edgeBias 用于判断是否进入硬转弯模式 */
    edgeBias = track_get_edge_bias(mask);

    /* 一旦检测到线，丢线计数清零，并允许下一次丢线再次计次 */
    gTrackLostTicks = 0U;
    gTrackLossCountedThisSession = 0U;


    /* ============================================================
     * 3. 宽线处理
     *
     * activeCount >= TRACK_WIDE_ACTIVE_THRESHOLD
     * 表示很多传感器同时压线：
     * - 可能进入交叉路口
     * - 可能进入宽黑区
     * - 可能进入特殊标志区
     *
     * 这种情况 measuredError 可能不可靠，
     * 于是使用 lastValidError 保持方向
     * ============================================================ */
    if (activeCount >= TRACK_WIDE_ACTIVE_THRESHOLD) {
        measuredError = gTrackLastValidError;
    }


    /* ============================================================
     * 4. error 滤波
     *
     * filteredError = blend(lastError, measuredError)
     *
     * TRACK_ERROR_FILTER_NUM / DEN 控制滤波强度：
     * - 越接近 0 -> 更平滑（反应慢）
     * - 越接近 DEN -> 更灵敏（噪声大）
     * ============================================================ */
    filteredError = track_blend16(gTrackLastError, measuredError,
        TRACK_ERROR_FILTER_NUM, TRACK_ERROR_FILTER_DEN);


    /* ============================================================
     * 5. 计算 baseSpeed（基础速度）
     *
     * error越大，baseSpeed越小，保证弯道稳定性
     * ============================================================ */
    baseSpeed = track_calc_base_speed(mask, activeCount, filteredError);

    /* 保存有效误差，用于丢线时推断方向 */
    gTrackLastValidError = filteredError;


    /* ============================================================
     * 6. 更新 lastTurn（最近转向方向）
     *
     * 用于丢线搜线时决定向左还是向右找线
     * ============================================================ */
    if (filteredError > 100) {
        gTrackLastTurn = 1;
    } else if (filteredError < -100) {
        gTrackLastTurn = -1;
    }


    /* ============================================================
     * 7. PD 控制计算
     *
     * deltaError = error - lastError
     *
     * targetControl = Kp*error + Kd*deltaError
     *
     * 其中 Kp 和 Kd 由宏定义：
     * Kp = TRACK_KP_NUM / TRACK_GAIN_DEN
     * Kd = TRACK_KD_NUM / TRACK_GAIN_DEN
     * ============================================================ */
    deltaError = (int16_t) (filteredError - gTrackLastError);

    targetControl = (int16_t) (((int32_t) TRACK_KP_NUM * filteredError +
                          (int32_t) TRACK_KD_NUM * deltaError) / TRACK_GAIN_DEN);


    /* 默认 control 变化限速 */
    stepLimit = (int16_t) TRACK_CONTROL_STEP_LIMIT;


    /* ============================================================
     * 8. 硬转弯模式（Hard Turn）
     *
     * 当 edgeBias != 0 表示小车偏到边缘，误差很大，
     * 需要额外的 boost 强制转回来。
     *
     * 进入硬转弯时：
     * - baseSpeed 限制更低（TRACK_HARD_TURN_SPEED）
     * - targetControl 增加 BOOST（TRACK_HARD_TURN_BOOST）
     * - stepLimit 改用更大值（TRACK_HARD_TURN_STEP_LIMIT）
     * ============================================================ */
    if (edgeBias != 0) {

        /* 硬转弯时速度限制在更低范围，提高稳定性 */
        baseSpeed = track_clamp16(baseSpeed,
            (int16_t) TRACK_HARD_TURN_SPEED,
            (int16_t) TRACK_BASE_SPEED);

        /* 加强纠偏力度（强制拉回轨道） */
        targetControl += (int16_t) edgeBias * (int16_t) TRACK_HARD_TURN_BOOST;

        /* 硬转弯时允许 control 变化更快 */
        stepLimit = (int16_t) TRACK_HARD_TURN_STEP_LIMIT;

        /*
         * 如果最外侧 OUT1/OUT8 触发，说明更危险，
         * 再追加一点 boost，防止冲出线
         */
        if ((mask & 0x81U) != 0U) {
            targetControl += (int16_t) edgeBias *
                (int16_t) (TRACK_HARD_TURN_BOOST / 2U);
        }
    }


    /* ============================================================
     * 9. targetControl 限幅
     *
     * 防止差速过大导致车抖动或原地旋转
     * ============================================================ */
    targetControl = track_clamp16(targetControl,
        -(int16_t) TRACK_MAX_CORRECTION,
        (int16_t) TRACK_MAX_CORRECTION);


    /* ============================================================
     * 10. control 滤波
     *
     * 这里对 targetControl 再做一阶滤波，
     * 进一步降低顿挫，提高弯道平滑度
     * ============================================================ */
    targetControl = track_blend16(gTrackLastControl, targetControl,
        TRACK_CONTROL_FILTER_NUM, TRACK_CONTROL_FILTER_DEN);


    /* ============================================================
     * 11. control 限速（变化速率限制）
     *
     * 防止 control 突然跳变，例如：
     * - 传感器抖动
     * - S弯误差翻转
     * - 反光导致误判
     * ============================================================ */
    control = track_limit_step(targetControl, gTrackLastControl, stepLimit);


    /* 再次限幅保证安全 */
    control = track_clamp16(control,
        -(int16_t) TRACK_MAX_CORRECTION,
        (int16_t) TRACK_MAX_CORRECTION);


    /* ============================================================
     * 12. 差速输出
     *
     * leftSpeed  = baseSpeed + control
     * rightSpeed = baseSpeed - control
     *
     * error > 0 -> control > 0 -> 左轮更快 -> 右转
     * error < 0 -> control < 0 -> 右轮更快 -> 左转
     * ============================================================ */
    leftSpeed = (int16_t) (baseSpeed + control);
    rightSpeed = (int16_t) (baseSpeed - control);

    track_apply_speed(leftSpeed, rightSpeed);


    /* ============================================================
     * 13. 保存状态变量供下一次循环使用
     * ============================================================ */
    gTrackLastControl = control;
    gTrackLastError = filteredError;
    gTrackLastLoopHadLine = 1U;
}