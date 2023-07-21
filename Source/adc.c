#include "adc.h"
#include <driverlib/driverlib.h>

#include <stdbool.h>

#include <arm_math_types_f16.h>

extern volatile uint8_t STATE_CODE;

static uint8_t DMA_CONTROL_TABLE[1024] __attribute__((aligned(1024)));

volatile float32_t SAMPLE_DATA[ADC_SAMPLE_SIZE];

/**
 * @brief 初始化ADC
 *
 */
void adc_init() {
  /* 配置GPIO */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P5, GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION
  ); // P5.5复用为Analog 0输入

  /* 配置ADC */
  MAP_ADC14_enableModule(); // 启用模块并初始化
  MAP_ADC14_initModule(ADC_CLOCKSOURCE_SMCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, ADC_NOROUTE);

  MAP_ADC14_configureSingleSampleMode(ADC_MEM0, true); // 配置采样模式模式
  MAP_ADC14_configureConversionMemory(
      ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, ADC_NONDIFFERENTIAL_INPUTS
  );
  MAP_ADC14_setSampleHoldTrigger(
      ADC_TRIGGER_SOURCE1, false
  ); // 配置触发源为Timer_A 0，上升沿触发。触发源对应参见SLAS826H, Table 6-51
  MAP_ADC14_enableConversion(); // 启动转换

  MAP_ADC14_clearInterruptFlag(ADC_INT0);
  MAP_ADC14_enableInterrupt(ADC_INT0); // 使能中断：ADC寄存器0偏移位置模数转换完成

  /* 配置DMA */
  MAP_DMA_enableModule();
  MAP_DMA_setControlBase(DMA_CONTROL_TABLE);

  MAP_DMA_disableChannelAttribute(
      DMA_CH7_ADC14,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK
  ); // 初始化并禁用所有属性

  MAP_DMA_assignChannel(DMA_CH7_ADC14); // 分配到Channel7 ADC, 外设对应参见SLAS826H, Table 6-36
  MAP_DMA_clearInterruptFlag(7);
  MAP_DMA_assignInterrupt(DMA_INT1, 7);

  MAP_Interrupt_enableInterrupt(INT_DMA_INT1);
}

/**
 * @brief 选择采样率
 *
 * @param freq 可选 1000, 500, 250, 125, 62/63, 25
 * @note 建议
 * 500  k ~ 125  k -> 1000  kHz
 * 125  k ~  50  k ->  500  kHz
 *  50  k ~  25  k ->  250  kHz
 *  25  k ~  12.5k ->  125  kHz
 *  12.5k ~   5  k ->   62.5kHz
 *   5  k ~   1  k ->   25  kHz
 */
void adc_run(uint16_t freq) {
  uint8_t divider;

  // 根据目标采样率设定分频系数
  switch (freq) {
  case 1000:
    divider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    break;
  case 500:
    divider = TIMER_A_CLOCKSOURCE_DIVIDER_2;
    break;
  case 250:
    divider = TIMER_A_CLOCKSOURCE_DIVIDER_4;
    break;
  case 125:
    divider = TIMER_A_CLOCKSOURCE_DIVIDER_8;
    break;
  case 62:
    divider = TIMER_A_CLOCKSOURCE_DIVIDER_10;
    break;
  case 25:
    divider = TIMER_A_CLOCKSOURCE_DIVIDER_40;
    break;
  }

  const Timer_A_PWMConfig pwn_sample_config = {
      .clockSource        = TIMER_A_CLOCKSOURCE_SMCLK,
      .clockSourceDivider = divider,
      .compareRegister    = TIMER_A_CAPTURECOMPARE_REGISTER_1,
      .compareOutputMode  = TIMER_A_OUTPUTMODE_SET_RESET,
      .timerPeriod        = 23,
      .dutyCycle          = 19}; // 1MHz 方波

  // 配置DMA双缓存转运。启动前，主副结构各配置1024次转运
  MAP_DMA_setChannelControl(
      DMA_CH7_ADC14 | UDMA_PRI_SELECT,
      UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1
  );
  MAP_DMA_setChannelTransfer(
      DMA_CH7_ADC14 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void *)&ADC14->MEM[0],
      (void *)(SAMPLE_DATA + 0), 1024
  );
  MAP_DMA_setChannelControl(
      DMA_CH7_ADC14 | UDMA_ALT_SELECT,
      UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1
  );
  MAP_DMA_setChannelTransfer(
      DMA_CH7_ADC14 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void *)&ADC14->MEM[0],
      (void *)(SAMPLE_DATA + 1024), 1024
  );

  // 启动ADC转换
  MAP_Timer_A_generatePWM(TIMER_A0_BASE, &pwn_sample_config);
  MAP_DMA_enableChannel(7);
}

/**
 * @brief 停止ADC采集
 *
 */
void adc_stop(void) {
  MAP_DMA_disableChannel(7);
  MAP_Timer_A_stopTimer(TIMER_A0_BASE);
}

/**
 * @brief 规范化ADC数据，转为半精度浮点数
 *
 */
void adc_sacle(void) {
  // 根据比例处理ADC数据
  for (uint16_t i = 0; i < ADC_SAMPLE_SIZE; i++) {
    SAMPLE_DATA[i] = (float32_t)(*(uint16_t *)(SAMPLE_DATA + i) / 128.0f - 64);
  } // TODO 公式调整
}

/**
 * @brief 处理DMA中断并重新配置转运。
 *
 */
void DMA_INT1_IRQHandler(void) {
  static uint8_t sample_repeat = 0;

  if (sample_repeat == 0) {
    //  初次中断，重新配置主结构
    MAP_DMA_setChannelControl(
        DMA_CH7_ADC14 | UDMA_PRI_SELECT,
        UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1
    );
    MAP_DMA_setChannelTransfer(
        DMA_CH7_ADC14 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void *)&ADC14->MEM[0],
        (void *)(SAMPLE_DATA + 2048), 1024
    );
    sample_repeat++;
  } else if (sample_repeat == 1) {
    // 二次中断，重新配置副结构
    MAP_DMA_setChannelControl(
        DMA_CH7_ADC14 | UDMA_ALT_SELECT,
        UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1
    );
    MAP_DMA_setChannelTransfer(
        DMA_CH7_ADC14 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void *)&ADC14->MEM[0],
        (void *)(SAMPLE_DATA + 3072), 1024
    );
    sample_repeat++;
  } else if (sample_repeat == 2) {
    // 三次中断，空过
    sample_repeat++;
  } else {
    // 四次中断，结束
    STATE_CODE = 0x00;

    sample_repeat = 0;
  }
}