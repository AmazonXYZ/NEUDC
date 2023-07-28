#include "vofa.h"

#include <math.h>
#include <stdbool.h>

#include <arm_math_types.h>
#include <arm_math_types_f16.h>

#include "serial.h"

static const uint8_t FRAME_TAIL[4] = {0x00, 0x00, 0x80, 0x7F};

/**
 * @brief 将单通道数据转换成符合JustFloat标准的数据并串口发送
 *
 * @param array 浮点数组
 * @param length 数据量
 * @param if16 是否为16位浮点数。默认为32位
 */
void vofa_justfloat_single(void *array, uint16_t length, bool if16) {
  if (if16) {
    float16_t *array_f16 = (float16_t *)array;

    float32_t tmp;
    for (uint16_t i = 0; i < length; i++) {
      tmp = (float32_t)array_f16[i];
      serial_send((uint8_t *)&tmp, 4);
      serial_send(FRAME_TAIL, 4);
    }
  } else {
    float32_t *array_f32 = (float32_t *)array;

    for (uint16_t i = 0; i < length; i++) {
      serial_send((uint8_t *)(array_f32 + i), 4);
      serial_send(FRAME_TAIL, 4);
    }
  }
}

/**
 * @brief 将双通道数据转换成符合Firewater标准的数据并串口发送
 *
 * @note 此格式也适合使用PySerial解析
 * @param array1 第一通道数据
 * @param array2 第二通道数据
 * @param length 数据长度
 * @param if16 是否为半精度浮点数
 */
void vofa_firewater_duo(void *array1, void *array2, uint16_t length, bool if16) {
  if (if16) {
    for (uint16_t i = 0; i < length; i++) {
      serial_printf("%f,%f\n", *((float16_t *)array1 + i), *((float16_t *)array2 + i));
    }
  } else {
    for (uint16_t i = 0; i < length; i++) {
      serial_printf("%f,%f\n", *((float32_t *)array1 + i), *((float32_t *)array2 + i));
    }
  }
}
