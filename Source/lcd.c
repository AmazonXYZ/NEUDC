#include "lcd.h"
#include <driverlib/driverlib.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <arm_math.h>
#include <arm_math_types.h>

#include "delay.h"
#include "serial.h"

extern volatile uint8_t STATE_CODE;
extern float16_t        AMPLITUDE[];
extern float16_t        PHASE[];
extern float16_t        THD;

static const eUSCI_UART_ConfigV1 uart_config = {
    .uartMode          = EUSCI_A_UART_MODE,              // 标准UART模式
    .selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK, // 子系统主时钟
    .clockPrescalar    = 13,                             // 时钟分频（计算得）
    .firstModReg       = 0,                              // （计算得）
    .secondModReg      = 37,                             // （计算得）
    .overSampling      = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION, // 标准超采
    .dataLength        = EUSCI_A_UART_8_BIT_LEN,                        // 八位数据长度
    .numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT,                     // 一停止位
    .msborLsbFirst     = EUSCI_A_UART_LSB_FIRST,                        // LSB顺序
    .parity            = EUSCI_A_UART_NO_PARITY,                        // 无校验位
};

static const uint8_t FRAME_TAIL[3] = "\xFF\xFF\xFF"; // TJC控制帧尾字符

/**
 * @brief LCD串口初始化
 *
 */
void lcd_init(void) {
  /* GPIO 配置 */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION
  ); // P2.2复用为RXD，P2.3复用为TXD

  /* UART 配置 */
  MAP_UART_initModule(EUSCI_A1_BASE, &uart_config);

  MAP_UART_enableModule(EUSCI_A1_BASE); // 将其作为UART模块使能

  MAP_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT); // 配置中断类型
  MAP_Interrupt_enableInterrupt(INT_EUSCIA1);                              // 使能EUSCIA0中断
}

/**
 * @brief 向LCD发送数据
 *
 * @param data 发送数据
 * @param length 数据长度
 */
void lcd_send(const uint8_t *data, uint16_t length) {
  while (length--) {
    MAP_UART_transmitData(EUSCI_A1_BASE, *data++);
  }
}

/**
 * @brief 向LCD发送字符串
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
    MAP_UART_transmitData(EUSCI_A1_BASE, fstring[offset]);
    offset++;
  }
}

/**
 * @brief 更新LCD显示数据
 *
 */
void lcd_refresh(void) {
  // 归一化幅值为百分比
  lcd_printf("HW2.val=%d%s", (uint16_t)round(AMPLITUDE[1] * 1e5 / AMPLITUDE[0]), FRAME_TAIL);
  lcd_printf("HW3.val=%d%s", (uint16_t)round(AMPLITUDE[2] * 1e5 / AMPLITUDE[0]), FRAME_TAIL);
  lcd_printf("HW4.val=%d%s", (uint16_t)round(AMPLITUDE[3] * 1e5 / AMPLITUDE[0]), FRAME_TAIL);
  lcd_printf("HW5.val=%d%s", (uint16_t)round(AMPLITUDE[4] * 1e5 / AMPLITUDE[0]), FRAME_TAIL);
  // 总谐波失真为原始值
  lcd_printf("THD.val=%d%s", (uint16_t)round(THD * 1e3), FRAME_TAIL);
}

/**
 * @brief 绘制波形图并发送到LED。滚屏，满屏一周期。
 *
 */
void lcd_graph(void) {
  // 检查是否还在波形界面
  while (STATE_CODE == 0x3) {
    for (uint16_t dot = 0; dot < 320; dot++) {
      float16_t y      = 0;
      float32_t radian = dot / 320.0f * 2 * PI;

      for (uint8_t i = 0; i < 5; i++) {
        y += AMPLITUDE[i] * arm_sin_f32((i + 1) * radian + PHASE[i]);
      }

      // 归一化
      y /= (AMPLITUDE[0] + AMPLITUDE[1] + AMPLITUDE[2] + AMPLITUDE[3] + AMPLITUDE[4]);
      // 规范化
      y = 240 / 2.0f * (y + 1);

      lcd_printf("add 1,0,%u%s", (uint8_t)round(y), FRAME_TAIL);
      delay_ms(15);
    }
  }
}

/**
 * @brief LCD触控中断
 *
 */
void EUSCIA1_IRQHandler(void) {

  /**
   * @brief 传输状态码
   * @param 0x1 开始采样
   * @param 0x02 返回主界面
   * @param 0x03 绘图请求
   */
  uint8_t trans_code;
  trans_code = MAP_UART_receiveData(EUSCI_A1_BASE);

  // 异常处理
  static uint8_t miss = 0;
  if (miss) {
    miss--;
    return;
  }

  switch (trans_code) {
  case 0x1:
    // 开始采样/重新采样
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