#ifndef TRACK_H
#define TRACK_H

#include <stdint.h>

/* ============================================================
 * @file    track.h
 * @brief   8路循迹控制模块头文件
 *
 * 本模块实现：
 *  - 8路循迹传感器采样
 *  - 误差计算（加权平均）
 *  - PD 控制（比例 + 微分）
 *  - error滤波 + control滤波 + 限速变化（防顿挫）
 *  - 丢线策略：直线行驶；第二次确认丢线后停车（不再原地旋转搜线）
 *  - 边缘硬转弯策略（Hard Turn）
 *
 * 适用场景：
 *  - 黑胶带循迹 / 白底循迹
 *  - 半圆弯道（例如半径40cm）
 *  - 塑料布反光环境（需要滤波和抗抖动）
 *
 * PWM说明：
 *  - 本项目电机PWM周期为 4000
 *  - 所有速度值范围推荐：0 ~ 4000
 * ============================================================ */


/* ============================================================
 * 基础配置参数
 * ============================================================ */

/**
 * @brief 循迹传感器数量（8路）
 *
 * 一般循迹模块输出8路数字信号：
 * OUT1 ~ OUT8 对应 8 个传感器。
 */
#define TRACK_SENSOR_COUNT       8U

/**
 * @brief 控制循环周期（ms）
 *
 * 你的 main() 中每隔 TRACK_LOOP_INTERVAL_MS 调用一次 Track_ControlStep()
 *
 * 建议范围：
 *  - 5ms  ：最常用，响应快，控制更细腻
 *  - 10ms ：更省资源，但弯道可能反应慢
 */
#define TRACK_LOOP_INTERVAL_MS   5U

/**
 * @brief 赛道黑线宽度（单位：mm）
 *
 * 用于算法理解（当前代码中没有直接用到该值）
 * 主要是做注释或未来扩展（如判断宽线/路口）。
 */
#define TRACK_LINE_WIDTH_MM      20U


/* ============================================================
 * 速度相关参数（PWM占空比控制）
 *
 * 说明：
 *  - 所有 speed 单位为 PWM比较值（0~4000）
 *  - 数值越大，PWM占空比越高，电机越快
 * ============================================================ */

/**
 * @brief 直道基础速度
 *
 * 车在直线正常循迹时使用的速度。
 *
 * 调参建议：
 *  - 想跑更快：增大（例如 2000~2500）
 *  - 想更稳：减小（例如 1500~1800）
 *
 * 注意：
 * 速度越高，弯道越容易冲出，需要配合降低 MAX_CORRECTION 或提高滤波。
 */
#define TRACK_BASE_SPEED         1950U

/**
 * @brief 弯道最低速度（算法 clamp 下限）
 *
 * baseSpeed 会根据误差动态下降，但不会低于 TRACK_CURVE_SPEED。
 *
 * 作用：
 *  - 弯道减速，提高转弯稳定性
 *
 * 调参建议：
 *  - 弯道冲出：降低（例如 700~900）
 *  - 弯道顿挫或电机不转：提高（例如 900~1300）
 */
#define TRACK_CURVE_SPEED        820U

/**
 * @brief 边缘纠偏速度
 *
 * 当车偏到边缘（OUT1/OUT8附近触发）时使用的速度。
 *
 * 作用：
 *  - 车即将跑出线时减速，保证有时间纠正回来
 */
#define TRACK_EDGE_SPEED         1000U

/**
 * @brief 硬转弯最低速度
 *
 * 当进入 Hard Turn 模式（严重偏离）时，
 * baseSpeed 会被 clamp 到 [TRACK_HARD_TURN_SPEED, TRACK_BASE_SPEED]
 *
 * Hard Turn 场景：
 *  - 车偏得很厉害
 *  - OUT1/OUT8单边触发
 *  - 需要强行快速拉回
 *
 * 调参建议：
 *  - 转弯冲过头：降低
 *  - 转弯拉不回来：提高一点
 */
#define TRACK_HARD_TURN_SPEED    620U

/**
 * @brief （保留）历史版本原地搜线速度，当前逻辑已不再使用
 */
#define TRACK_SEARCH_SPEED       760U

/**
 * @brief 丢线后直线行驶速度（左右轮同速）
 */
#define TRACK_LOST_STRAIGHT_SPEED  1000U

/**
 * @brief 确认一次“丢线事件”所需的连续丢线周期数（抗抖）
 *
 * 循环为 TRACK_LOOP_INTERVAL_MS 时，3 周期约 15ms。
 */
#define TRACK_LOST_EVENT_CONFIRM_TICKS  3U

/**
 * @brief 最大速度限制
 *
 * 所有输出给电机的速度都会 clamp 到 [-TRACK_MAX_SPEED, TRACK_MAX_SPEED]
 *
 * 作用：
 *  - 防止某些情况下 control 太大导致速度超过合理范围
 */
#define TRACK_MAX_SPEED          2400U

/**
 * @brief 最大纠偏差速（control最大值）
 *
 * control 输出会被 clamp 在 [-TRACK_MAX_CORRECTION, TRACK_MAX_CORRECTION]
 *
 * control 作用：
 *  leftSpeed  = baseSpeed + control
 *  rightSpeed = baseSpeed - control
 *
 * 调参建议：
 *  - 转弯能力不够，拉不回来：增大
 *  - 车左右摆动太大 / 很顿挫：减小
 *
 * 注意：
 * MAX_CORRECTION 太大时可能导致某一侧轮子变成负数（反转），车会抖或原地转。
 */
#define TRACK_MAX_CORRECTION     1650U

/**
 * @brief 丢线短时间恢复时使用的纠偏控制量
 *
 * mask==0 丢线后，不立刻原地旋转，
 * 而是先滑行几次，并施加一个固定差速，让车朝上次方向回归。
 *
 * 这个值越大：
 *  - 丢线时回线更快
 *  - 但可能误判时导致突然猛转
 */
#define TRACK_LOST_RECOVERY_CORRECTION  320U

/**
 * @brief Hard Turn 模式下额外增加的纠偏力度
 *
 * edgeBias != 0 时触发 Hard Turn
 * targetControl 会额外加上：
 *   edgeBias * TRACK_HARD_TURN_BOOST
 *
 * boost 越大：
 *  - 拉回更猛
 *  - 但顿挫感更强，可能造成甩尾
 */
#define TRACK_HARD_TURN_BOOST    420U


/* ============================================================
 * PD 控制参数（核心调参）
 *
 * PD公式：
 *   control = (Kp * error + Kd * deltaError) / TRACK_GAIN_DEN
 *
 * error      : 当前偏差
 * deltaError : 当前偏差 - 上一次偏差
 *
 * ============================================================ */

/**
 * @brief 比例系数 Kp 分子
 *
 * 实际 Kp = TRACK_KP_NUM / TRACK_GAIN_DEN
 *
 * Kp 越大：
 *  - 纠偏越强
 *  - 转弯更积极
 *  - 但直线更容易左右摆动
 */
#define TRACK_KP_NUM             32

/**
 * @brief 微分系数 Kd 分子
 *
 * 实际 Kd = TRACK_KD_NUM / TRACK_GAIN_DEN
 *
 * Kd 作用：
 *  - 抑制快速变化
 *  - 提前预测弯道趋势
 *
 * Kd 越大：
 *  - 抑制过冲更强
 *  - 但容易放大噪声（塑料布反光时会抖）
 */
#define TRACK_KD_NUM             13

/**
 * @brief PD 系数分母（统一缩放系数）
 *
 * 实际：
 *  Kp = TRACK_KP_NUM / TRACK_GAIN_DEN
 *  Kd = TRACK_KD_NUM / TRACK_GAIN_DEN
 */
#define TRACK_GAIN_DEN           100


/* ============================================================
 * 滤波参数（抗抖动、抗反光、减少顿挫）
 * ============================================================ */

/**
 * @brief error滤波比例（分子）
 *
 * error滤波采用 track_blend16()
 *
 * filteredError = lastError*(DEN-NUM)/DEN + measuredError*NUM/DEN
 *
 * 当前配置 NUM=3 DEN=4
 * -> 当前误差占比 75%，上一帧误差占比 25%
 *
 * NUM 越小：滤波越强，反应更慢（更稳）
 * NUM 越大：反应更快，但更抖
 */
#define TRACK_ERROR_FILTER_NUM   4U

/**
 * @brief error滤波分母
 */
#define TRACK_ERROR_FILTER_DEN   5U

/**
 * @brief control滤波比例（分子）
 *
 * 用于让 control 输出更平滑，减少弯道顿挫。
 *
 * 当前 NUM=3 DEN=4 -> 新 control 占 75%，响应更快
 */
#define TRACK_CONTROL_FILTER_NUM 3U

/**
 * @brief control滤波分母
 */
#define TRACK_CONTROL_FILTER_DEN 4U

/**
 * @brief control变化最大步长（软限速）
 *
 * 每次控制循环中，control 的变化不会超过此值。
 *
 * 作用：
 * - 防止误差突然跳变导致猛打方向
 * - 防止塑料布反光造成瞬间抖动
 *
 * 值越小：
 *  - 更平滑、更稳
 *  - 但转弯响应变慢
 *
 * 值越大：
 *  - 转弯更灵敏
 *  - 但可能顿挫更明显
 */
#define TRACK_CONTROL_STEP_LIMIT 260U

/**
 * @brief Hard Turn 模式下 control 变化步长
 *
 * Hard Turn 时允许更快变化，否则拉不回来。
 */
#define TRACK_HARD_TURN_STEP_LIMIT 340U


/* ============================================================
 * 丢线处理参数
 * ============================================================ */

/**
 * @brief （保留）历史版本丢线滑行周期数，当前逻辑已不再使用
 */
#define TRACK_LOST_GLIDE_TICKS   4U


/* ============================================================
 * 弯道/宽线检测阈值参数
 * ============================================================ */

/**
 * @brief 弯道检测阈值（传感器触发数量阈值）
 *
 * activeCount >= TRACK_CURVE_ACTIVE_THRESHOLD 时，
 * 认为进入弯道或大角度区域，需要减速。
 *
 * activeCount 表示当前 mask 中有多少位为 1。
 *
 * 值越小：
 * - 更容易进入弯道减速（更稳）
 * 值越大：
 * - 更不容易减速（更快，但更容易冲出弯）
 */
#define TRACK_CURVE_ACTIVE_THRESHOLD  5U

/**
 * @brief 宽线/路口检测阈值
 *
 * activeCount >= TRACK_WIDE_ACTIVE_THRESHOLD 时，
 * 认为进入宽线区域（例如路口、十字、黑色区域）
 *
 * 这时 measuredError 可能不可信，会使用 lastValidError。
 */
#define TRACK_WIDE_ACTIVE_THRESHOLD   6U


/* ============================================================
 * 外部接口函数声明
 * ============================================================ */

/**
 * @brief 初始化循迹模块状态变量
 *
 * 必须在主程序启动时调用一次。
 */
void Track_Init(void);

/**
 * @brief 循迹控制主函数（核心）
 *
 * 建议按照 TRACK_LOOP_INTERVAL_MS 周期调用。
 * 每次调用都会更新电机速度，实现循迹。
 */
void Track_ControlStep(void);

/**
 * @brief 读取当前8路传感器状态 mask
 *
 * bit0->XIN1 ... bit7->XIN8
 *
 * 黑线输出 = 1
 * 白底输出 = 0
 */
uint8_t Track_Read_State(void);

/**
 * @brief 获取上一次控制周期的误差值（调试用）
 *
 * 返回的是滤波后的误差值。
 */
int16_t Track_GetLastError(void);

#endif /* TRACK_H */