/**
 * @file    led_status.h
 * @brief   D1 (PWR) / D2 (FAULT) LED control - plain GPIO on U2.
 */
#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <stdbool.h>

void led_status_init(void);
void led_pwr_set(bool on);
void led_fault_set(bool on);
void led_fault_blink_step(void); /* call periodically from main loop/tick */

#endif /* LED_STATUS_H */
