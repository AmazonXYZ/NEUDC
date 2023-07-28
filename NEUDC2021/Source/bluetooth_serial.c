#include "bluetooth_serial.h"
#include <driverlib/driverlib.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

static const uint8_t port_mapping[] = {
    // 将EUSCI_A1_SPI_UART.RXD/TXD 映射到 P2.6/P2.4
    PM_NONE, PM_NONE, PM_NONE, PM_NONE, PM_UCA1TXD, PM_NONE, PM_UCA1RXD, PM_NONE,
};

static const eUSCI_UART_ConfigV1 uart_config = {
    .uartMode          = EUSCI_A_UART_MODE,              // 标准UART模式
    .selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK, // 子系统主时钟
    .clockPrescalar    = 13,                             // 时钟分频（计算得）
    .firstModReg       = 0,                              // 对应波特率（计算得）
    .secondModReg      = 37,                             // 9600（计算得）
    .overSampling      = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION, // 标准超采
    .dataLength        = EUSCI_A_UART_8_BIT_LEN,                        // 八位数据长度
    .numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT,                     // 一停止位
    .msborLsbFirst     = EUSCI_A_UART_LSB_FIRST,                        // LSB顺序
    .parity            = EUSCI_A_UART_NO_PARITY,                        // 无校验位
};

void bluetooth_init(void) {
  /* PMAP 配置 */
  MAP_PMAP_configurePorts(port_mapping, PMAP_P2MAP, 1, PMAP_DISABLE_RECONFIGURATION);
  /* GPIO 配置 */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P2, GPIO_PIN6 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION
  ); // P2.6复用为RXD，P2.4复用为TXD

  /* UART 配置 */
  MAP_UART_initModule(EUSCI_A1_BASE, &uart_config);

  MAP_UART_enableModule(EUSCI_A1_BASE); // 将其作为UART模块使能
}

/// @brief 通过蓝牙串口发送数据。请先和手机配对。
/// @param format
/// @param
void bluetooth_printf(char *format, ...) {
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