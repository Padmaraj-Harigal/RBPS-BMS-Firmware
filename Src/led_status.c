/**
 * @file    led_status.c
 * @brief   D1/D2 LED driver. Replace the GPIO_PORT/GPIO_PIN macros below
 *          with the actual CubeMX-generated names for your net (see
 *          bms_config.h GPIO_PIN_LED_* symbolic names for the mapping
 *          back to schematic references D1/D2).
 */
#include "led_status.h"
#include "stm32c0xx_hal.h"

/* These are placeholders only. The PCB LED nets are not connected to
 * MCU GPIO pads; update only after the board wiring has been corrected. */
#define LED_PWR_PORT     GPIOA
#define LED_PWR_PIN      GPIO_PIN_4
#define LED_FAULT_PORT   GPIOA
#define LED_FAULT_PIN    GPIO_PIN_5

static bool s_fault_blink_state = false;

void led_status_init(void)
{
    led_pwr_set(true);   /* power LED on as soon as firmware is alive */
    led_fault_set(false);
}

void led_pwr_set(bool on)
{
    HAL_GPIO_WritePin(LED_PWR_PORT, LED_PWR_PIN,
                       on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_fault_set(bool on)
{
    HAL_GPIO_WritePin(LED_FAULT_PORT, LED_FAULT_PIN,
                       on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    s_fault_blink_state = on;
}

void led_fault_blink_step(void)
{
    s_fault_blink_state = !s_fault_blink_state;
    HAL_GPIO_WritePin(LED_FAULT_PORT, LED_FAULT_PIN,
                       s_fault_blink_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
