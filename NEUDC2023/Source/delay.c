#include "delay.h"
#include <driverlib/driverlib.h>

#include <stdint.h>

#define DELAY_PERIOD_1MS 375

static uint32_t count_down;

/**
 * @brief 初始化Timer_A 1计时器，用于延时
 *
 */
void delay_init() {
  const Timer_A_UpModeConfig delay_config = {
      .clockSource        = TIMER_A_CLOCKSOURCE_SMCLK,      // 时钟源
      .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64, // 分频系数
      .timerClear         = TIMER_A_DO_CLEAR,               // 中断后清除计数
      .timerPeriod        = DELAY_PERIOD_1MS,               // CCR0计数值
      .captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // 打开CCR0中断
  };
  MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &delay_config);
  MAP_Interrupt_enableInterrupt(INT_TA1_0); // 使能TA1_0中断
}

/**
 * @brief 延时ms毫秒
 *
 * @param ms 最大为 2^32 - 1 ms ~ 50 day
 */
void delay_ms(uint32_t ms) {
  count_down = ms;
  MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
  while (count_down) {
    __no_operation();
  }
  MAP_Timer_A_stopTimer(TIMER_A1_BASE);
}

void TA1_0_IRQHandler(void) {
  MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
  count_down--;
}