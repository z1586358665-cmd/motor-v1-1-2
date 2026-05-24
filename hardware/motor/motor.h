// motor.h
#ifndef _MOTOR_H
#define _MOTOR_H

#include <stdint.h>

// 电机编号
#define MOTOR_A 0
#define MOTOR_B 1
#define MOTOR_C 2
#define MOTOR_D 3

// 电机方向
#define MOTOR_CW  1   // 正转
#define MOTOR_CCW 0   // 反转

// 速度范围
#define MOTOR_SPEED_MIN 0
#define MOTOR_SPEED_MAX 3200

/*
 * 单电机 PWM 比例微调（千分比，1000 = 不补偿）
 *
 * 同指令下四轮实际转速仍可能不同（电机差异、摩擦、安装），
 * 走直线若明显跑偏，把偏快一侧的 GAIN 略减小（如 970），偏慢侧略增大（如 1030），每次改 10～30 微调。
 */
#define MOTOR_PWM_GAIN_A  1000U
#define MOTOR_PWM_GAIN_B  1000U
#define MOTOR_PWM_GAIN_C  1000U
#define MOTOR_PWM_GAIN_D  1000U

// 初始化所有电机
void Motor_Init(void);

// 控制单个电机
// motor_id: MOTOR_A, MOTOR_B, MOTOR_C, MOTOR_D
// dir: MOTOR_CW 或 MOTOR_CCW
// speed: 0-1000
void Motor_Control(uint8_t motor_id, uint8_t dir, uint16_t speed);

// 同时控制所有电机（用于小车前进、后退、转向）
void Motor_ControlAll(uint8_t dir_a, uint16_t speed_a,
                      uint8_t dir_b, uint16_t speed_b,
                      uint8_t dir_c, uint16_t speed_c,
                      uint8_t dir_d, uint16_t speed_d);

// 便捷控制函数
void Motor_Forward(uint16_t speed);     // 所有电机正转（前进）
void Motor_Backward(uint16_t speed);    // 所有电机反转（后退）
void Motor_TurnLeft(uint16_t speed);    // 左转（右边电机正转，左边反转）
void Motor_TurnRight(uint16_t speed);   // 右转（左边电机正转，右边反转）
void Motor_Stop(void);                  // 停止所有电机

// 单独控制某侧电机
void Motor_LeftSide(uint8_t dir, uint16_t speed);   // 左边电机A和C
void Motor_RightSide(uint8_t dir, uint16_t speed);  // 右边电机B和D

#endif