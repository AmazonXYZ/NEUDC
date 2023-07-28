#include "serial.h"
#include <driverlib/driverlib.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <arm_math_types.h>
#include <arm_math_types_f16.h>

/**
 * @brief baudrate.js
 */
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

void serial_init(void) {
  /* GPIO 配置 */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION
  ); // P1.2复用为RXD，P1.3复用为TXD

  /* UART 配置 */
  MAP_UART_initModule(EUSCI_A0_BASE, &uart_config);

  MAP_UART_enableModule(EUSCI_A0_BASE); // 将其作为UART模块使能

  MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT); // 配置中断类型
  MAP_Interrupt_enableInterrupt(INT_EUSCIA0);                              // 使能EUSCIA0中断
}

void serial_send(const uint8_t *data, uint16_t length) {
  while (length--) {
    MAP_UART_transmitData(EUSCI_A0_BASE, *data++);
  }
}

void serial_printf(char *format, ...) {
  char fstring[128];

  va_list arg;
  va_start(arg, format);
  vsprintf(fstring, format, arg);
  va_end(arg);

  uint16_t offset = 0;

  while (fstring[offset] != 0) {
    MAP_UART_transmitData(EUSCI_A0_BASE, fstring[offset]);
    offset++;
  }
}

void EUSCIA0_IRQHandler(void) {
  /**
   * @brief 传输状态码
   * @param 0x1 固定应答
   * @param 0x2 状态码传入
   */
  static uint8_t trans_code = 0x00;

  switch (trans_code) {
  case 0x0:
    trans_code = MAP_UART_receiveData(EUSCI_A0_BASE);
    if (trans_code == 0x1) {
      serial_printf("OK\n");
      trans_code = 0x0;
    } else if (trans_code != 0x2) {
      serial_printf("Bad Code\n");
      trans_code = 0x00;
    }
    break;

  case 0x02:
    STATE_CODE = MAP_UART_receiveData(EUSCI_A0_BASE);
    trans_code = 0;
    break;
  }
}