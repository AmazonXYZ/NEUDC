#include "adc.h"
#include "arm_math_types.h"
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
 * @brief 启动ADC
 *
 * @param target_freq 信号频率
 * @return 实际采样频率
 */
void adc_run(float32_t target_freq, float32_t *sample_freq_addr) {
  uint16_t period;
  if (target_freq > 31.25e3) {
    period = 23; // 取目标分辨率为2e-7*f，若过大则降低采样频率
  } else {
    period = round(3e6 / 4 / target_freq) - 1;
  } // 此时最大采样时间为750时钟周期

  *sample_freq_addr = 24e6 / (period + 1.0f); // 返回实际采样频率

  /**
   * @brief 配置Timer_A PWM
   * @note 参考 SLAS826H Table 5-28, SLAU356I Table 22-1,Table 19-2,Figure 19-12
   * 此配置方法中，dutyCycle实际为低电平输出周期。
   * 由于外置触发源默认为拓展采样模式，这个值不应小于18，所以取19。
   * 理论最小采样时间不大于1us，最大采样时间不大于420us（对应10080时钟周期）
   */
  const Timer_A_PWMConfig pwn_sample_config = {
      .clockSource        = TIMER_A_CLOCKSOURCE_SMCLK, // 24MHz SMCLK
      .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1,
      .compareRegister    = TIMER_A_CAPTURECOMPARE_REGISTER_1,
      .compareOutputMode  = TIMER_A_OUTPUTMODE_SET_RESET,
      .timerPeriod        = period,
      .dutyCycle          = 19};

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
    SAMPLE_DATA[i] = (float32_t)(*(uint16_t *)(SAMPLE_DATA + i) / 128.0f - 63.99609375);
  }
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
    SAMPLE_FLAG = false;

    sample_repeat = 0;
  }
}