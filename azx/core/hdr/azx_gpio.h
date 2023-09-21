/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_72d2f54f_61cb_4404_b2f3_4553b5b5b03d
#define UUID_72d2f54f_61cb_4404_b2f3_4553b5b5b03d
/**
 * @file azx_gpio.h
 * @version 1.0.3
 * @dependencies core/azx_log core/azx_utils core/azx_timer
 * @author Ioannis Demetriou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Interact with the modem's GPIO pins
 *
 * This provides a wrapper over the m2mb_gpio.h API that simplifies how the
 * interaction of the GPIO pins is made. You can just read and write to the
 * needed pin through azx_gpio_set() and azx_gpio_get().
 *
 * If you need to receive a callback on interrupt, use
 * azx_gpio_registerInterrupt().
 *
 * You don't need to call azx_gpio_conf(), but if you want to manually configure
 * the pin, then you can use this.
 *
 * For full flexibility use the m2mb_gpio.h API directly.
 */
#include "m2mb_types.h"
#include "m2mb_gpio.h"
#include "azx_log.h"
#include "azx_utils.h"

#define AZX_GPIO_HIGH 1 /**< GPIO is in HIGH state*/
#define AZX_GPIO_LOW  0 /**< GPIO is in LOW state*/

#define AZX_GPIO_READ_ERROR 0xFF /**< GPIO reading ERROR */

/**
 * As of FW 30.00.XX6-B002 there has been a correction to the typo in the GPIO trigger enum.
 * Therefore if you use a FW version which is >= XX6-B002, define USING_UPDATE_M2MB_GPIO.
 */
#ifndef USING_UPDATED_M2MB_GPIO
#define AZX_GPIO_TRIGGER_E M2MB_GPIO__TRIGGER_E
#define AZX_MAX_ENUM_M2MB_GPIO_TRIGGER_E MAX_ENUM_M2MB_GPIO__TRIGGER_E
#else
#define AZX_GPIO_TRIGGER_E M2MB_GPIO_TRIGGER_E
#define AZX_MAX_ENUM_M2MB_GPIO_TRIGGER_E MAX_ENUM_M2MB_GPIO_TRIGGER_E
#endif

const CHAR* azx_gpio_printTrigger(AZX_GPIO_TRIGGER_E trigger);
#define strgpiotrig(trigger) azx_gpio_printTrigger(trigger)

const CHAR* azx_gpio_printDirection(M2MB_GPIO_DIRECTION_E direction);
#define strgpiodir(direction) azx_gpio_printDirection(direction)

const CHAR* azx_gpio_printPull(M2MB_GPIO_PULL_MODE_E pull);
#define strgpiopull(pull) azx_gpio_printPull(pull)

/**
 * @brief Configure the GPIO before usage.
 *
 * Note: This doesn't need to be called for azx_gpio_set() and azx_gpio_get().
 * The library is going to configure everything automatically.
 *
 * @param[in] gpio The pin to configure
 * @param[in] direction Either @ref M2MB_GPIO_MODE_OUTPUT or @ref M2MB_GPIO_MODE_OUTPUT
 * @param[in] pull The pull mode to use
 *
 * @return `TRUE` on success, `FALSE` otherwise
 */
BOOLEAN azx_gpio_conf_ex(UINT8 gpio, M2MB_GPIO_DIRECTION_E direction, M2MB_GPIO_PULL_MODE_E pull);
#define azx_gpio_conf(gpio, direction) azx_gpio_conf_ex(gpio, direction, M2MB_GPIO_PULL_KEEPER)

/**
 * @brief Sets a GPIO as output with the provided status.
 *
 * If the file descriptor is already opened, it will be reused.
 *
 * @param[in] gpio The pin to set
 * @param[in] status 1 for on, 0 for off
 *
 * @return `TRUE` on success, `FALSE` otherwise
 */
BOOLEAN azx_gpio_set(UINT8 gpio, UINT8 status);

/**
 * @brief Gets the status of a GPIO pin.
 *
 * If the file descriptor is already opened, it will be reused.
 *
 * @param[in] gpio The pin to read
 *
 * @return One of:
 *     @ref AZX_GPIO_HIGH for on,
 *     @ref AZX_GPIO_LOW for off,
 *     @ref AZX_GPIO_READ_ERROR for error
 */
UINT8 azx_gpio_get(UINT8 gpio);

/**
 * @brief Sends a square pulse.
 *
 * Caveat: the pulses generation is managed via the OS, 
 * so accuracy may vary greatly depending on system load.
 *
 * @param[in] gpio The pin to send the pulse to
 * @param[in] period Duration of period in usecs
 * @param[in] no_of_waves How many waves to send
 *
 * @return `TRUE` if the call succeeds, `FALSE` otherwise.
 */
BOOLEAN azx_gpio_sendPulse(UINT8 gpio, UINT32 period, UINT32 no_of_waves);

/**
 * @brief Signature of the callback function when interrupt is triggered.
 *
 * @param[in] gpio The pin where the interrupt triggered.
 * @param[in] state The new state of the GPIO (AZX_GPIO_HIGH or AZX_GPIO_LOW).
 *
 * @see azx_gpio_registerInterrupt
 */
typedef void azx_gpio_interrupt_cb(UINT8 gpio, UINT8 state);

/**
 * @brief Sets the interrupt handling for a GPIO pin.
 *
 * @param[in] gpio The pin to register an interrupt
 * @param[in] trigger The type of trigger. One of
 *     M2MB_GPIO_INTR_POSEDGE (Rising edge, trigger when the pin goes from low to high,
 *     M2MB_GPIO_INTR_NEGEDGE (Falling edge)
 *     M2MB_GPIO_INTR_ANYEDGE, triggered whenever the pin changes value
 * @param[in] pull_mode One of the modes defined in M2MB_GPIO_PULL_MODE_E.
 * @param[in] cb The function to call
 * @param[in] debounce_cooldown_interval_ms Ignores any new interrupts from the same
 *     GPIO if they arrive within the specified cooldown period.
 *     - Recommended debouncing interval: `300`ms
 *     - Use `0` for no debouncing.
 *
 * @return `TRUE` if the call succeeds, `FALSE` otherwise.
 *
 * @see azx_gpio_interrupt_cb
 */
BOOLEAN azx_gpio_registerInterrupt(UINT8 gpio, AZX_GPIO_TRIGGER_E trigger,
    M2MB_GPIO_PULL_MODE_E pull_mode,
    azx_gpio_interrupt_cb* cb, UINT32 debounce_cooldown_interval_ms);

#endif /* !defined( UUID_72d2f54f_61cb_4404_b2f3_4553b5b5b03d ) */
