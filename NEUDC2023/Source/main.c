#include "main.h"
#include <driverlib/driverlib.h>

#include <stdbool.h>

int main(void) {
  MAP_WDT_A_holdTimer();
  main_init();

  while (true) {
    __no_operation();
  }
  return 0;
}

// void gpio_init(void) {
//   /* GPIO配置*/
//   // TODO
//   __no_operation();
// }

/// @brief 初始化全局状态配置
void main_init(void) {
  MAP_WDT_A_holdTimer(); // 关闭看门狗

  /* 供电配置 */
  MAP_PCM_setPowerMode(PCM_DCDC_MODE);     // 设置DCDC供电模式（高负载）
  MAP_PCM_setCoreVoltageLevel(PCM_VCORE1); // 设置核心高压（高性能）

  /* 内存配置 */
  MAP_FlashCtl_setWaitState(FLASH_BANK0, 1); // 设置Flash等待状态为1
  MAP_FlashCtl_setWaitState(FLASH_BANK1, 1); // 读写速度拉满 // TODO 文档 ?

  /* 时钟配置 参考 SLAU356I Table 8-1 */
  MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);    // 中心频率 拉满 48MHz
  MAP_CS_setReferenceOscillatorFrequency(CS_REFO_128KHZ); // 参考时钟 拉满 128kHz

  MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1); // 主时钟 内部晶振 拉满
  MAP_CS_initClockSignal(
      CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_2
  ); // 低速子系统主时钟 内部晶振2分频 24MHz
  MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1); // 辅频时钟 参考时钟 拉满

  /* FPU配置 */
  MAP_FPU_enableModule();       // 使能FPU
  MAP_FPU_enableLazyStacking(); // 开启惰性堆栈，提高中断响应速度

  MAP_Interrupt_enableMaster(); // 使能总中断
}