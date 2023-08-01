#include "ad9954.h"
#include <driverlib/driverlib.h>
#include <reg_util.h>

#include <stdbool.h>
#include <stdint.h>

#define AD9954_OSC_FREQ       20e6 // 20e6
#define AD9954_OSC_MULTIPLIER 20   // 4 ~ 20

#define AD9954_FREQ (AD9954_OSC_FREQ * AD9954_OSC_MULTIPLIER)

#define AD9954_REG_CFR1 0x00
#define AD9954_REG_CFR2 0x01
#define AD9954_REG_ASF  0x02
#define AD9954_REG_ARR  0x03
#define AD9954_REG_FTW0 0x04
#define AD9954_REG_POW0 0x05
#define AD9954_REG_FTW1 0x06

/* 扫频模式 */
#define AD9954_REG_NLSCW 0x07
#define AD9954_REG_PLSCW 0x08

/* 正常模式*/
#define AD9954_REG_RSCW0 0x07
#define AD9954_REG_RSCW1 0x08
#define AD9954_REG_RSCW2 0x09
#define AD9954_REG_RSCW3 0x0A
#define AD9954_REG_RAM   0x0B

static union DataBuffer {
  uint64_t lsb;
  uint8_t  byte[8];
} receiver;

static uint8_t offset = 0;

static const eUSCI_SPI_MasterConfig spi_config = {
    EUSCI_SPI_CLOCKSOURCE_SMCLK,                           // 参考时钟源
    24e6,                                                  // 参考时钟源频率
    16e6,                                                  // 目标频率
    EUSCI_B_SPI_MSB_FIRST,                                 // MSB顺序
    EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT, // 第一个边沿捕获，第二个边沿更新
    EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_LOW,                // 时钟闲时低电平
    EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW                     // 四线低电平工作模式
};

/// @brief 计算频率调谐字。FTW =  2**32 * 目标频率 / 时钟频率
/// @param freq 目标频率
/// @return 频率调谐字
inline static uint32_t ad9954_freq2ftw(double freq) {
  return (uint32_t)(freq * (1 << 32) / AD9954_FREQ);
}

inline static void ad9954_send_byte(uint8_t byte) {
  MAP_SPI_transmitData(EUSCI_B0_BASE, byte);
}

static void ad9954_send(uint8_t *data, uint8_t length) {
  while (length--) {
    ad9954_send_byte(*data++);
  }
}

void ad9954_write(uint8_t address, uint8_t *data, uint8_t length) {
  ad9954_send_byte(address);
  ad9954_send(data, length);
}

uint32_t ad9954_read(uint8_t address, uint8_t length) {
  ad9954_send_byte(address);
  while (offset < length) {
    __no_operation();
  }
  offset = 0;

  uint32_t result = 0;
  for (uint8_t i = 0; i < length; i++) {
    result <<= 8;
    result += receiver.byte[i];
  }

  return result;
}

inline void ad9964_update(void) {
  // TODO
  // MAP_GPIO_toggleOutputOnPin(GPIO_PORT_Px, GPIO_PINx);
  // MAP_GPIO_toggleOutputOnPin(GPIO_PORT_Px, GPIO_PINx);
}

/// @brief AD9954初始化。默认关闭方波输出
void ad9954_init(void) {
  /* GPIO 配置 SPI接口 */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P1, GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION
  );

  /* GPIO 配置 AD9954控制接口 */
  // TODO
  // MAP_GPIO_setOutputLowOnPin(GPIO_PORT_Px, GPIO_PINx); // UPD
  // MAP_GPIO_setAsOutputPin(GPIO_PORT_Py, GPIO_PINy); // PS1
  // MAP_GPIO_setAsOutputPin(GPIO_PORT_Pz, GPIO_PINz); // PS2

  /* SPI 配置 */
  MAP_SPI_initMaster(EUSCI_B0_BASE, &spi_config);
  MAP_SPI_selectFourPinFunctionality(EUSCI_B0_BASE, EUSCI_B_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE);

  MAP_SPI_enableModule(EUSCI_B1_BASE);

  MAP_SPI_enableInterrupt(EUSCI_B0_BASE, EUSCI_SPI_RECEIVE_INTERRUPT);
  MAP_Interrupt_enableInterrupt(INT_EUSCIB0);

  union DataBuffer cfr1_config = {
      .byte = {BYTE(0, 0, 0, 0, 0, 0, 1, 0), 0x00, 0x00, BYTE(0, 1, 0, 0, 0, 0, 0, 0)}};

  union DataBuffer cfr2_config = {.byte = {0x00, 0x00, AD9954_OSC_MULTIPLIER << 3}};

  if (AD9954_FREQ > 250e6) {
    cfr2_config.byte[2] |= BYTE(0, 0, 0, 0, 0, 1, 0, 0);
  }

  ad9954_write(AD9954_REG_CFR1, cfr1_config.byte, 4);
  ad9954_write(AD9954_REG_CFR1, cfr1_config.byte, 3);
}

/// @brief 设置PS1和PS2引脚电平
/// @param ps1 PS1是否置高
/// @param ps2 PS2是否置高
void ad9954_set_ps_mode(bool ps1, bool ps2) {
  if (ps1) {
    // MAP_GPIO_setOutputHighOnPin(GPIO_PORT_Px, GPIO_PINx);
  } else {
    // MAP_GPIO_setOutputLowOnPin(GPIO_PORT_Px, GPIO_PINx);
  }
  // TODO
  if (ps2) {
    // MAP_GPIO_setOutputHighOnPin(GPIO_PORT_Px, GPIO_PINx);
  } else {
    // MAP_GPIO_setOutputLowOnPin(GPIO_PORT_Px, GPIO_PINx);
  }
}

/// @brief 设置DDS输出频率，不能大于其晶振频率，不能大于140e6
/// @param freq 目标频率
void ad9954_set_freq(float freq) {
  union DataBuffer ftw0_config;
  ftw0_config.lsb = ad9954_freq2ftw(freq);

  TOGGLE_BYTE_ORDER_32(ftw0_config.byte);
  ad9954_write(AD9954_REG_FTW0, ftw0_config.byte, 4);
}

/// @brief 设置输出幅值
/// @param amp_ratio 0 ~ 1的幅值比例
void ad9954_set_amp(float amp_ratio) {
  union DataBuffer asf_config = {(uint16_t)(amp_ratio * 0x3FFF)};

  TOGGLE_BYTE_ORDER_16(asf_config.byte);
  ad9954_write(AD9954_REG_ASF, &asf_config, 2);
}

/// @brief 设置输出相位
/// @param phase_radio 0 ~ 1的相位比例
void ad9954_set_phase(float phase_radio) {
  union DataBuffer pow0_config = {(uint16_t)(phase_radio * 0x3FFF)};

  TOGGLE_BYTE_ORDER_16(pow0_config.byte);
  ad9954_write(AD9954_REG_POW0, &pow0_config, 2);
}

/// @brief 设置线性扫频
/// @param start_freq 起始频率
/// @param stop_freq 截止频率。必须大于起始频率
/// @param step_freq 扫频步进。0 ~ 140E6
/// @param step_time_d10 十分之一步进时间，单位为纳秒
/// @param mode 工作模式
void ad9954_set_linearsweep(
    float start_freq, float stop_freq, float step_freq, uint8_t step_time_d10, uint8_t mode
) {
  union DataBuffer cfr1_config = {.byte = {0x00, BYTE(0, 0, 1, 0, 0, 0, 0, 0), 0x00, mode}};

  ad9954_write(AD9954_REG_CFR1, cfr1_config.byte, 4);

  union DataBuffer ftw0_config, ftw1_config, nlscw_config, plscw_config;

  ftw0_config.lsb = ad9954_freq2ftw(start_freq);
  TOGGLE_BYTE_ORDER_32(ftw0_config.byte);

  ftw1_config.lsb = ad9954_freq2ftw(stop_freq);
  TOGGLE_BYTE_ORDER_32(ftw1_config.byte);

  nlscw_config.lsb                     = step_time_d10;
  *(uint32_t *)(nlscw_config.byte + 1) = ad9954_freq2ftw(step_freq);
  TOGGLE_BYTE_ORDER_32(ftw1_config.byte + 1);

  plscw_config.lsb                     = step_time_d10;
  *(uint32_t *)(plscw_config.byte + 1) = ad9954_freq2ftw(step_freq);
  TOGGLE_BYTE_ORDER_32(ftw1_config.byte + 1);

  ad9954_write(AD9954_REG_FTW0, ftw0_config.byte, 4);
  ad9954_write(AD9954_REG_FTW1, ftw1_config.byte, 4);
  ad9954_write(AD9954_REG_NLSCW, nlscw_config.byte, 5);
  ad9954_write(AD9954_REG_PLSCW, plscw_config.byte, 5);
}

void EUSCIB0_IRQHandler(void) {
  receiver.byte[offset++] = SPI_receiveData(EUSCI_B0_BASE);
}
