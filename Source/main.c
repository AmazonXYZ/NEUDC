/**
 * @file main.c
 * @author AmazonXYZ (jqy1977017245@gmail.com)
 * @brief NEUDC 2021 A Solution. Require Arm Compiler for Embedded.
 * @version 0.1
 * @date 2023-07-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#include <driverlib/driverlib.h>

#include <stdbool.h>
#include <stdint.h>

#include <arm_math_types_f16.h>

#include "adc.h"
#include "delay.h"
#include "fft.h"
#include "lcd.h"
#include "led.h"
#include "process.h"
#include "serial.h"

/* Debug */
#include "pwm.h"
#include "vofa.h"

/**
 * @brief 状态码
 *
 * @param 0x00: 无状态/空闲
 * @param 0x01: 采样运行
 * @param 0x02: 采样结束/等待
 * @param 0x03: 图像输出
 */
volatile uint8_t STATE_CODE = 0x00;

int main(void) {
  main_init();

  adc_init();
  delay_init();
  fft_init();
  lcd_init();
  serial_init();

  led_alert_short();

  /* Debug Start */
  pwm_init();
  /* Debug End */

  while (1) {
    switch (STATE_CODE) {
    case 0x00:
      // 空闲等待
      __no_operation();
      break;
    case 0x01:
      // 采样开启
      // preprocess();

      // // 基频校准，额外做一次
      // process(0);
      // process(0);

      // process(1);
      // process(2);
      // process(3);
      // process(4);

      /* Debug Start */
      pwm_run();
      update(1000);
      vofa_firewater_duo((void *)FFT_OUTPUT, (void *)FFT_PHASE, ADC_SAMPLE_SIZE / 2 + 1, false);
      /* Debug End */

      // AMPLITUDE[0] = 1;
      // AMPLITUDE[4] = 0.1;
      // THD          = 0.1;

      // lcd_refresh();
      STATE_CODE = 0x02;
    case 0x02:
      // 结束等待
      __no_operation();
      break;
    case 0x03:
      // 绘图请求
      lcd_graph();
      break;
    }
  }
}

/**
 * @brief 调整至高性能状态，配置基本功能
 *
 */
void main_init(void) {
  MAP_WDT_A_holdTimer(); // 关闭看门狗

  /* 供电配置 */
  MAP_PCM_setPowerMode(PCM_DCDC_MODE);     // 设置DCDC供电模式（高负载）
  MAP_PCM_setCoreVoltageLevel(PCM_VCORE1); // 设置核心高压（高性能）

  /* 内存配置 */
  MAP_FlashCtl_setWaitState(FLASH_BANK0, 1); // 设置Flash等待状态为1 // TODO 文档哪写了？
  MAP_FlashCtl_setWaitState(FLASH_BANK1, 1); // 读写速度拉满

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