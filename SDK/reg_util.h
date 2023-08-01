#pragma once

/// @brief LSB Order
#define BYTE(bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0)                   \
  ((uint8_t)(((bit7) << 7) | ((bit6) << 6) | ((bit5) << 5) | ((bit4) << 4) |   \
             ((bit3) << 3) | ((bit2) << 2) | ((bit1) << 1) | (bit0)))

#define SET_BIT(reg, bit)    ((reg) |= (1U << (bit)))
#define GET_BIT(reg, bit)    ((reg) & (1U << (bit)))
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(1U << (bit)))
#define TOGGLE_BIT(reg, bit) ((reg) ^= (1U << (bit)))

#define LOW_BITS(bytes, offset)    ((bytes) & ((1U << (offset)) - 1))
#define HIGHER_BITS(bytes, offset) ((bytes) >> (offset))

#define TOGGLE_BYTE_ORDER_16(bytes)                                            \
  do {                                                                         \
    uint8_t _;                                                                 \
    _          = (bytes)[0];                                                   \
    (bytes)[0] = (bytes)[1];                                                   \
    (bytes)[1] = _;                                                            \
  } while (0)

#define TOGGLE_BYTE_ORDER_32(bytes)                                            \
  do {                                                                         \
    uint8_t _;                                                                 \
    _          = (bytes)[0];                                                   \
    (bytes)[0] = (bytes)[3];                                                   \
    (bytes)[3] = _;                                                            \
    _          = (bytes)[1];                                                   \
    (bytes)[1] = (bytes)[2];                                                   \
    (bytes)[2] = _;                                                            \
  } while (0)

#ifdef BITS64

#define LOW_BITS(bytes, offset) ((bytes) & ((1UL << (offset)) - 1))
#define TOGGLE_BYTE_ORDER_64(bytes)                                            \
  do {                                                                         \
    uint8_t _;                                                                 \
    _          = (bytes)[0];                                                   \
    (bytes)[0] = (bytes)[7];                                                   \
    (bytes)[7] = _;                                                            \
    _          = (bytes)[1];                                                   \
    (bytes)[1] = (bytes)[6];                                                   \
    (bytes)[6] = _;                                                            \
    _          = (bytes)[2];                                                   \
    (bytes)[2] = (bytes)[5];                                                   \
    (bytes)[5] = _;                                                            \
    _          = (bytes)[3];                                                   \
    (bytes)[3] = (bytes)[4];                                                   \
    (bytes)[4] = _;                                                            \
  } while (0)

#endif
