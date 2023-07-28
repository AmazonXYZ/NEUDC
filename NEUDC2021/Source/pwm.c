#include "pwm.h"
#include <driverlib/driverlib.h>

static const Timer_A_PWMConfig pwm_signal_config = {
    .clockSource        = TIMER_A_CLOCKSOURCE_ACLK,
    .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1,
    .compareRegister    = TIMER_A_CAPTURECOMPARE_REGISTER_1,
    .compareOutputMode  = TIMER_A_OUTPUTMODE_RESET_SET,
    .timerPeriod        = 31,
    .dutyCycle          = 16}; // 4kHz 方波

void pwm_init(void) {
  MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P5, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);
}

void pwm_run(void) { MAP_Timer_A_generatePWM(TIMER_A2_BASE, &pwm_signal_config); }

void pwm_stop(void) { MAP_Timer_A_stopTimer(TIMER_A2_BASE); }