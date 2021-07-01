/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_9d96c69f_2f8c_4f4e_9a28_f8047959c63e
#define UUID_9d96c69f_2f8c_4f4e_9a28_f8047959c63e
/**
 * @file azx_i2c.h
 * @version 1.0.1
 * @dependencies core/azx_log
 * @author Ioannis Demetriou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Communicate with peripherals over the I2C bus
 *
 * Use this API to quickly read from a properly connected I2C sensor using
 * azx_i2c_read(), and write to an I2C sensor using azx_i2c_write(). All the
 * underlying details are handled by the library.
 *
 * @note By default it is assumed the SDA is on GPIO 2, and SCL on GPIO 3. If you have a different
 * hardware configuration, use azx_i2c_set_pins to set the correct lines.
 *
 * For full flexibility use the m2mb_i2c.h API directly.
 *
 * @see azx_i2c_read
 * @see azx_i2c_write
 */
#include "m2mb_types.h"
#include "azx_log.h"

/**
 * @brief Set the pins to be used for the I2C communication.
 *
 * This should be called only once based on the HW configuration and all the I2C data will be going
 * through those pins.
 *
 * @param sda The pin to be used for the data line
 * @param scl The pin to be used for the clock line
 */
void azx_i2c_set_pins(UINT8 sda, UINT8 scl);

/**
 * @brief Reads from an I2C device.
 *
 * @param[in] device_id The ID of the device
 * @param[in] register_id The ID of the register
 * @param[out] buffer The buffer to write to
 * @param[in] size How many bytes to read
 *
 * @return `TRUE` if the read was successful, `FALSE` otherwise
 * */
BOOLEAN azx_i2c_read(UINT16 device_id, UINT8 register_id,
    UINT8* buffer, UINT32 size);

/**
 * @brief Writes to an I2C device.
 *
 * @param[in] device_id The ID of the device
 * @param[in] register_id The ID of the register
 * @param[in] buffer The buffer to write out
 * @param[in] size How many bytes to write
 *
 * @return `TRUE` if the read was successful, `FALSE` otherwise
 * */
BOOLEAN azx_i2c_write(UINT16 device_id, UINT8 register_id,
    const UINT8* buffer, UINT32 size);

/**
 * @brief Writes and reads from an I2C device without using any register.
 *
 * Normally you want to use azx_i2c_read and azx_i2c_write, but for some devices you are required to
 * perform I/O operations without using a register. In those cases this function can be used.
 *
 * @param[in] device_id The ID of the device
 * @param[in] out The buffer to send
 * @param[in] out_size How many bytes to send
 * @param[out] in The buffer to store the response
 * @param[in] in_size How many bytes to read
 *
 * @return `TRUE` if the read was successful, `FALSE` otherwise
 * */
BOOLEAN azx_i2c_raw_read_write(UINT16 device_id,
    const UINT8* out, UINT32 out_size,
    UINT8* in, UINT32 in_size);

#endif /* !defined( UUID_9d96c69f_2f8c_4f4e_9a28_f8047959c63e ) */
