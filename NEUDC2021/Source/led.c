#include "led.h"
#include <driverlib/driverlib.h>

#include <delay.h>

/**
 * @brief 红灯常亮
 *
 */
void led_alert_hold(void) {
  MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
  while (1) {
    __no_operation();
  }
}

/**
 * @brief 红灯亮0.2s
 *
 */
void led_alert_short(void) {
  MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
  delay_ms(200);
  MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
}