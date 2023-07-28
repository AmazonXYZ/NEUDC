#include "main.h"
#include <driverlib/driverlib.h>

#include <stdbool.h>
#include <stdint.h>

#include <arm_math_types.h>

#include "adc.h"
#include "bluetooth_serial.h"
#include "delay.h"
#include "fft.h"
#include "lcd_serial.h"
#include "led.h"
#include "msp432p401r.h"
#include "process.h"
#include "serial.h"

/**
 * @brief 状态码
 *
 * @param 0x00: 无状态/空闲
 * @param 0x01: 采样运行
 * @param 0x02: 采样结束/等待
 * @param 0x03: 图像输出
 */
volatile uint8_t STATE_CODE = 0x00;

volatile bool RESAMPLE = false;

int main(void) {
  main_init();

  adc_init();
  bluetooth_init();
  delay_init();
  fft_init();
  lcd_init();

  led_alert_short();

#ifdef DEBUG
  serial_init();
#endif

  while (1) {
    switch (STATE_CODE) {
    case 0x00:
      // 空闲等待
      __no_operation();
      break;

    case 0x01:
      // 采样开启
      RESAMPLE = false;
      preprocess();
    case 0x02:
      // 采样循环运行
      process(0);
      process(1);
      process(2);
      process(3);
      process(4);

      if (STATE_CODE == 0x01) {
        STATE_CODE = 0x02;
      }
      // 更新lcd数据
      lcd_refresh();

      // 更新蓝牙数据
      bluetooth_printf("<%f,%f,%f,%f,%f:a>", hw_amp[0], hw_amp[1], hw_amp[2], hw_amp[3], hw_amp[4]);

      if (RESAMPLE) {
        STATE_CODE = 0x01;
        break;
      }

      break;
    case 0x03:
      // 绘图请求
      lcd_graph();
      break;
    case 0x04:
      // Debug 专用
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
  MAP_FlashCtl_setWaitState(FLASH_BANK0, 1); // 设置Flash等待状态为1 // TODO 文档?
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