#include "vofa.h"

#include <math.h>
#include <stdbool.h>

#include <arm_math_types.h>
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
    float32_t *array_f16 = (float32_t *)array;

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

void vofa_firewater_duo(void *array1, void *array2, uint16_t length, bool if16) {
  if (if16) {
    for (uint16_t i = 0; i < length; i++) {
      serial_printf("%f,%f\n", *((float32_t *)array1 + i), *((float32_t *)array2 + i));
    }
  } else {
    for (uint16_t i = 0; i < length; i++) {
      serial_printf("%f,%f\n", *((float32_t *)array1 + i), *((float32_t *)array2 + i));
    }
  }
}
