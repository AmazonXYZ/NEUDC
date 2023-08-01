#include "lcd_serial.h"
#include <driverlib/driverlib.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <arm_math.h>
#include <arm_math_types.h>

#include "delay.h"
#include "serial.h"

extern volatile bool    RESAMPLE;
extern volatile uint8_t STATE_CODE;
extern float32_t        hw_amp[];
extern float32_t        hw_phase[];
extern float32_t        THD;

static const eUSCI_UART_ConfigV1 uart_config = {
    .uartMode          = EUSCI_A_UART_AUTOMATIC_BAUDRATE_DETECTION_MODE, // 自适应波特率
    .selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK,                 // 子系统主时钟
    .clockPrescalar    = 0,
    .firstModReg       = 0,
    .secondModReg      = 0,
    .overSampling      = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION, // 标准超采
    .dataLength        = EUSCI_A_UART_8_BIT_LEN,                        // 八位数据长度
    .numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT,                     // 一停止位
    .msborLsbFirst     = EUSCI_A_UART_LSB_FIRST,                        // LSB顺序
    .parity            = EUSCI_A_UART_NO_PARITY,                        // 无校验位
};

/**
 * @brief LCD串口初始化
 *
 */
void lcd_init(void) {
  /* GPIO 配置 */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION
  ); // P3.2复用为RXD，P3.3复用为TXD

  /* UART 配置 */
  MAP_UART_initModule(EUSCI_A2_BASE, &uart_config);

  MAP_UART_enableModule(EUSCI_A2_BASE); // 将其作为UART模块使能

  MAP_UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT); // 配置中断类型
  MAP_Interrupt_enableInterrupt(INT_EUSCIA2);                              // 使能EUSCIA2中断
}

/**
 * @brief 向LCD发送字符串，自动添加帧尾控制字符
 *
 * @param format 格式化字符串
 * @param ... 格式化内容
 */
void lcd_printf(char *format, ...) {
  char fstring[128];

  va_list arg;
  va_start(arg, format);
  vsprintf(fstring, format, arg);
  va_end(arg);

  uint16_t offset = 0;

  while (fstring[offset] != 0) {
    MAP_UART_transmitData(EUSCI_A2_BASE, fstring[offset]);
    offset++;
  }

  for (uint8_t i = 0; i < 3; i++) {
    MAP_UART_transmitData(EUSCI_A2_BASE, 0xFF);
  } // TJC控制帧尾字符
}

/**
 * @brief 更新LCD显示数据
 *
 */
void lcd_refresh(void) {
  // 百分比
  lcd_printf("HW2.val=%d", (uint16_t)round(hw_amp[1] / hw_amp[0] * 1e5));
  delay_ms(10);
  lcd_printf("HW3.val=%d", (uint16_t)round(hw_amp[2] / hw_amp[0] * 1e5));
  delay_ms(10);
  lcd_printf("HW4.val=%d", (uint16_t)round(hw_amp[3] / hw_amp[0] * 1e5));
  delay_ms(10);
  lcd_printf("HW5.val=%d", (uint16_t)round(hw_amp[4] / hw_amp[0] * 1e5));
  delay_ms(10);

  float32_t _THD;
  arm_rms_f32(hw_amp + 1, 4, &_THD);

  lcd_printf("THD.val=%d", (uint16_t)round(_THD / hw_amp[0] * 4e5));
}

/**
 * @brief 绘制波形图并发送到LED。滚屏，满屏一周期。
 *
 */
void lcd_graph(void) {
  // 检查是否还在波形界面
  while (STATE_CODE == 0x3) {
    for (uint16_t x = 0; x < 320; x++) {
      float32_t y      = 0;
      float32_t radian = x / 320.0f * 2 * PI;

      for (uint8_t i = 0; i < 5; i++) {
        y += hw_amp[i] * arm_sin_f32((i + 1) * radian + hw_phase[i]);
      }

      // 归一化
      y /= (hw_amp[0] + hw_amp[1] + hw_amp[2] + hw_amp[3] + hw_amp[4]);
      // 规范化
      y = 240 / 2.0f * (y + 1);

      lcd_printf("add 1,0,%u", (uint8_t)round(y));
      delay_ms(15);
    }
  }
}

/**
 * @brief LCD触控中断
 *
 */
void EUSCIA2_IRQHandler(void) {

  /**
   * @brief 传输状态码
   * @param 0x1 开始采样
   * @param 0x02 返回主界面
   * @param 0x03 绘图请求
   */
  uint8_t trans_code;
  trans_code = MAP_UART_receiveData(EUSCI_A2_BASE);

  // 异常处理
  static uint8_t miss = 0;
  if (miss) {
    miss--;
    return;
  }

  switch (trans_code) {
  case 0x1:
    // 开始采样/重新采样
    RESAMPLE   = true;
    STATE_CODE = 0x1;
    break;

  case 0x3:
    // 检查是否完成了采样和分析，若是则进入绘图状态
    if (STATE_CODE == 0x2) {
      STATE_CODE = 0x3;
    }
    break;
  case 0x2:
    // 检查是否处于绘图状态。若是则回到等待，否则回到待机
    if (STATE_CODE == 0x3) {
      STATE_CODE = 0x2;

    } else {
      STATE_CODE = 0x0;
    }
    break;
  default:
    // 异常处理
    miss = 3;
    break;
  }
}