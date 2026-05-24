/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gPWM_ABackup;
DL_TimerG_backupConfig gPWM_BBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_PWM_A_init();
    SYSCFG_DL_PWM_B_init();
    SYSCFG_DL_PWM_D_init();
    SYSCFG_DL_TIMER_SCH_init();
    /* Ensure backup structures have no valid state */
	gPWM_ABackup.backupRdy 	= false;
	gPWM_BBackup.backupRdy 	= false;


}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(PWM_A_INST, &gPWM_ABackup);
	retStatus &= DL_TimerG_saveConfiguration(PWM_B_INST, &gPWM_BBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(PWM_A_INST, &gPWM_ABackup, false);
	retStatus &= DL_TimerG_restoreConfiguration(PWM_B_INST, &gPWM_BBackup, false);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(PWM_A_INST);
    DL_TimerG_reset(PWM_B_INST);
    DL_TimerG_reset(PWM_D_INST);
    DL_TimerG_reset(TIMER_SCH_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(PWM_A_INST);
    DL_TimerG_enablePower(PWM_B_INST);
    DL_TimerG_enablePower(PWM_D_INST);
    DL_TimerG_enablePower(TIMER_SCH_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_A_C1_IOMUX,GPIO_PWM_A_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_A_C1_PORT, GPIO_PWM_A_C1_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_B_C0_IOMUX,GPIO_PWM_B_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_B_C0_PORT, GPIO_PWM_B_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_B_C1_IOMUX,GPIO_PWM_B_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_B_C1_PORT, GPIO_PWM_B_C1_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_D_C1_IOMUX,GPIO_PWM_D_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_D_C1_PORT, GPIO_PWM_D_C1_PIN);

    DL_GPIO_initDigitalOutput(MOTOR_AIN1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_AIN2_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_BIN1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_BIN2_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_CIN1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_CIN2_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_DIN1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_DIN2_IOMUX);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN5_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN6_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN7_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK_XIN8_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearPins(GPIOA, MOTOR_AIN1_PIN);
    DL_GPIO_enableOutput(GPIOA, MOTOR_AIN1_PIN);
    DL_GPIO_clearPins(GPIOB, MOTOR_AIN2_PIN |
		MOTOR_BIN1_PIN |
		MOTOR_BIN2_PIN |
		MOTOR_CIN1_PIN |
		MOTOR_CIN2_PIN |
		MOTOR_DIN1_PIN |
		MOTOR_DIN2_PIN);
    DL_GPIO_enableOutput(GPIOB, MOTOR_AIN2_PIN |
		MOTOR_BIN1_PIN |
		MOTOR_BIN2_PIN |
		MOTOR_CIN1_PIN |
		MOTOR_CIN2_PIN |
		MOTOR_DIN1_PIN |
		MOTOR_DIN2_PIN);

}



SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();

}


/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gPWM_AClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerA_PWMConfig gPWM_AConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 4000,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_A_init(void) {

    DL_TimerA_setClockConfig(
        PWM_A_INST, (DL_TimerA_ClockConfig *) &gPWM_AClockConfig);

    DL_TimerA_initPWMMode(
        PWM_A_INST, (DL_TimerA_PWMConfig *) &gPWM_AConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerA_setCounterControl(PWM_A_INST,DL_TIMER_CZC_CCCTL1_ZCOND,DL_TIMER_CAC_CCCTL1_ACOND,DL_TIMER_CLC_CCCTL1_LCOND);

    DL_TimerA_setCaptureCompareOutCtl(PWM_A_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_ENABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_1_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_A_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_A_INST, 4000, DL_TIMER_CC_1_INDEX);

    DL_TimerA_enableClock(PWM_A_INST);


    
    DL_TimerA_setCCPDirection(PWM_A_INST , DL_TIMER_CC1_OUTPUT );


}
/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_BClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_BConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 4000,
    .isTimerWithFourCC = true,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_B_init(void) {

    DL_TimerG_setClockConfig(
        PWM_B_INST, (DL_TimerG_ClockConfig *) &gPWM_BClockConfig);

    DL_TimerG_initPWMMode(
        PWM_B_INST, (DL_TimerG_PWMConfig *) &gPWM_BConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(PWM_B_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(PWM_B_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_ENABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_B_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_B_INST, 4000, DL_TIMER_CC_0_INDEX);

    DL_TimerG_setCaptureCompareOutCtl(PWM_B_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_ENABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_B_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_B_INST, 4000, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(PWM_B_INST);


    
    DL_TimerG_setCCPDirection(PWM_B_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );


}
/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_DClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_DConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 4000,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_D_init(void) {

    DL_TimerG_setClockConfig(
        PWM_D_INST, (DL_TimerG_ClockConfig *) &gPWM_DClockConfig);

    DL_TimerG_initPWMMode(
        PWM_D_INST, (DL_TimerG_PWMConfig *) &gPWM_DConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(PWM_D_INST,DL_TIMER_CZC_CCCTL1_ZCOND,DL_TIMER_CAC_CCCTL1_ACOND,DL_TIMER_CLC_CCCTL1_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(PWM_D_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_ENABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_D_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_D_INST, 4000, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(PWM_D_INST);


    
    DL_TimerG_setCCPDirection(PWM_D_INST , DL_TIMER_CC1_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1000000 Hz = 32000000 Hz / (1 * (31 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_SCHClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 31U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_SCH_INST_LOAD_VALUE = (10ms * 1000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_SCHTimerConfig = {
    .period     = TIMER_SCH_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_SCH_init(void) {

    DL_TimerG_setClockConfig(TIMER_SCH_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_SCHClockConfig);

    DL_TimerG_initTimerMode(TIMER_SCH_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_SCHTimerConfig);
    DL_TimerG_enableClock(TIMER_SCH_INST);





}


