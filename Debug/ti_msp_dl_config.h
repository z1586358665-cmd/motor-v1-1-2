/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_A */
#define PWM_A_INST                                                         TIMA1
#define PWM_A_INST_IRQHandler                                   TIMA1_IRQHandler
#define PWM_A_INST_INT_IRQN                                     (TIMA1_INT_IRQn)
#define PWM_A_INST_CLK_FREQ                                             32000000
/* GPIO defines for channel 1 */
#define GPIO_PWM_A_C1_PORT                                                 GPIOA
#define GPIO_PWM_A_C1_PIN                                         DL_GPIO_PIN_16
#define GPIO_PWM_A_C1_IOMUX                                      (IOMUX_PINCM38)
#define GPIO_PWM_A_C1_IOMUX_FUNC                     IOMUX_PINCM38_PF_TIMA1_CCP1
#define GPIO_PWM_A_C1_IDX                                    DL_TIMER_CC_1_INDEX

/* Defines for PWM_B */
#define PWM_B_INST                                                         TIMG6
#define PWM_B_INST_IRQHandler                                   TIMG6_IRQHandler
#define PWM_B_INST_INT_IRQN                                     (TIMG6_INT_IRQn)
#define PWM_B_INST_CLK_FREQ                                             32000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_B_C0_PORT                                                 GPIOB
#define GPIO_PWM_B_C0_PIN                                         DL_GPIO_PIN_10
#define GPIO_PWM_B_C0_IOMUX                                      (IOMUX_PINCM27)
#define GPIO_PWM_B_C0_IOMUX_FUNC                     IOMUX_PINCM27_PF_TIMG6_CCP0
#define GPIO_PWM_B_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_B_C1_PORT                                                 GPIOB
#define GPIO_PWM_B_C1_PIN                                          DL_GPIO_PIN_7
#define GPIO_PWM_B_C1_IOMUX                                      (IOMUX_PINCM24)
#define GPIO_PWM_B_C1_IOMUX_FUNC                     IOMUX_PINCM24_PF_TIMG6_CCP1
#define GPIO_PWM_B_C1_IDX                                    DL_TIMER_CC_1_INDEX

/* Defines for PWM_D */
#define PWM_D_INST                                                         TIMG8
#define PWM_D_INST_IRQHandler                                   TIMG8_IRQHandler
#define PWM_D_INST_INT_IRQN                                     (TIMG8_INT_IRQn)
#define PWM_D_INST_CLK_FREQ                                             32000000
/* GPIO defines for channel 1 */
#define GPIO_PWM_D_C1_PORT                                                 GPIOA
#define GPIO_PWM_D_C1_PIN                                         DL_GPIO_PIN_30
#define GPIO_PWM_D_C1_IOMUX                                       (IOMUX_PINCM5)
#define GPIO_PWM_D_C1_IOMUX_FUNC                      IOMUX_PINCM5_PF_TIMG8_CCP1
#define GPIO_PWM_D_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for TIMER_SCH */
#define TIMER_SCH_INST                                                   (TIMG0)
#define TIMER_SCH_INST_IRQHandler                               TIMG0_IRQHandler
#define TIMER_SCH_INST_INT_IRQN                                 (TIMG0_INT_IRQn)
#define TIMER_SCH_INST_LOAD_VALUE                                        (9999U)




/* Defines for AIN1: GPIOA.11 with pinCMx 22 on package pin 57 */
#define MOTOR_AIN1_PORT                                                  (GPIOA)
#define MOTOR_AIN1_PIN                                          (DL_GPIO_PIN_11)
#define MOTOR_AIN1_IOMUX                                         (IOMUX_PINCM22)
/* Defines for AIN2: GPIOB.3 with pinCMx 16 on package pin 51 */
#define MOTOR_AIN2_PORT                                                  (GPIOB)
#define MOTOR_AIN2_PIN                                           (DL_GPIO_PIN_3)
#define MOTOR_AIN2_IOMUX                                         (IOMUX_PINCM16)
/* Defines for BIN1: GPIOB.16 with pinCMx 33 on package pin 4 */
#define MOTOR_BIN1_PORT                                                  (GPIOB)
#define MOTOR_BIN1_PIN                                          (DL_GPIO_PIN_16)
#define MOTOR_BIN1_IOMUX                                         (IOMUX_PINCM33)
/* Defines for BIN2: GPIOB.12 with pinCMx 29 on package pin 64 */
#define MOTOR_BIN2_PORT                                                  (GPIOB)
#define MOTOR_BIN2_PIN                                          (DL_GPIO_PIN_12)
#define MOTOR_BIN2_IOMUX                                         (IOMUX_PINCM29)
/* Defines for CIN1: GPIOB.14 with pinCMx 31 on package pin 2 */
#define MOTOR_CIN1_PORT                                                  (GPIOB)
#define MOTOR_CIN1_PIN                                          (DL_GPIO_PIN_14)
#define MOTOR_CIN1_IOMUX                                         (IOMUX_PINCM31)
/* Defines for CIN2: GPIOB.11 with pinCMx 28 on package pin 63 */
#define MOTOR_CIN2_PORT                                                  (GPIOB)
#define MOTOR_CIN2_PIN                                          (DL_GPIO_PIN_11)
#define MOTOR_CIN2_IOMUX                                         (IOMUX_PINCM28)
/* Defines for DIN1: GPIOB.9 with pinCMx 26 on package pin 61 */
#define MOTOR_DIN1_PORT                                                  (GPIOB)
#define MOTOR_DIN1_PIN                                           (DL_GPIO_PIN_9)
#define MOTOR_DIN1_IOMUX                                         (IOMUX_PINCM26)
/* Defines for DIN2: GPIOB.8 with pinCMx 25 on package pin 60 */
#define MOTOR_DIN2_PORT                                                  (GPIOB)
#define MOTOR_DIN2_PIN                                           (DL_GPIO_PIN_8)
#define MOTOR_DIN2_IOMUX                                         (IOMUX_PINCM25)
/* Defines for XIN1: GPIOB.20 with pinCMx 48 on package pin 19 */
#define TRACK_XIN1_PORT                                                  (GPIOB)
#define TRACK_XIN1_PIN                                          (DL_GPIO_PIN_20)
#define TRACK_XIN1_IOMUX                                         (IOMUX_PINCM48)
/* Defines for XIN2: GPIOB.25 with pinCMx 56 on package pin 27 */
#define TRACK_XIN2_PORT                                                  (GPIOB)
#define TRACK_XIN2_PIN                                          (DL_GPIO_PIN_25)
#define TRACK_XIN2_IOMUX                                         (IOMUX_PINCM56)
/* Defines for XIN3: GPIOA.25 with pinCMx 55 on package pin 26 */
#define TRACK_XIN3_PORT                                                  (GPIOA)
#define TRACK_XIN3_PIN                                          (DL_GPIO_PIN_25)
#define TRACK_XIN3_IOMUX                                         (IOMUX_PINCM55)
/* Defines for XIN4: GPIOA.27 with pinCMx 60 on package pin 31 */
#define TRACK_XIN4_PORT                                                  (GPIOA)
#define TRACK_XIN4_PIN                                          (DL_GPIO_PIN_27)
#define TRACK_XIN4_IOMUX                                         (IOMUX_PINCM60)
/* Defines for XIN5: GPIOB.24 with pinCMx 52 on package pin 23 */
#define TRACK_XIN5_PORT                                                  (GPIOB)
#define TRACK_XIN5_PIN                                          (DL_GPIO_PIN_24)
#define TRACK_XIN5_IOMUX                                         (IOMUX_PINCM52)
/* Defines for XIN6: GPIOA.22 with pinCMx 47 on package pin 18 */
#define TRACK_XIN6_PORT                                                  (GPIOA)
#define TRACK_XIN6_PIN                                          (DL_GPIO_PIN_22)
#define TRACK_XIN6_IOMUX                                         (IOMUX_PINCM47)
/* Defines for XIN7: GPIOA.24 with pinCMx 54 on package pin 25 */
#define TRACK_XIN7_PORT                                                  (GPIOA)
#define TRACK_XIN7_PIN                                          (DL_GPIO_PIN_24)
#define TRACK_XIN7_IOMUX                                         (IOMUX_PINCM54)
/* Defines for XIN8: GPIOA.26 with pinCMx 59 on package pin 30 */
#define TRACK_XIN8_PORT                                                  (GPIOA)
#define TRACK_XIN8_PIN                                          (DL_GPIO_PIN_26)
#define TRACK_XIN8_IOMUX                                         (IOMUX_PINCM59)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_A_init(void);
void SYSCFG_DL_PWM_B_init(void);
void SYSCFG_DL_PWM_D_init(void);
void SYSCFG_DL_TIMER_SCH_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
