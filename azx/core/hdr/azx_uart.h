/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef LIBS_HDR_UART_H_
#define LIBS_HDR_UART_H_
/**
 * @file azx_uart.h
 * @version 1.0.2
 * @dependencies core/azx_log
 * @author Ioannis Demetriou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Communicate with devices via UART
 *
 * This library simplifies the procedure to communicate with a sensor using
 * UART.
 *
 * To use the library you can just call azx_uart_open() and then read/write to
 * the UART through azx_uart_read() and azx_uart_write().
 *
 * In addition, the library allows registering a callback function which will
 * be called when the UART buffer has data. The callback, with signature
 * @ref azx_uart_data_available_cb, is to be registered when opening UART.
 *
 * For full flexibility use the m2mb_uart.h API directly.
 */
#include "m2mb_types.h"
#include "azx_log.h"

/**
 * @brief Callback which indicates that some UART data is available.
 *
 * This call does not provide the data, but just notifies that there is some
 * data available.
 *
 * @warning Do **NOT** issue any azx_uart_read() from within this callback
 *     as the underlying UART library does not allow it.
 *
 * @param[in] nbyte The number of bytes available
 * @param[in] ctx The context as provided in azx_uart_open()
 *
 * @see azx_uart_open
 */
typedef void (*azx_uart_data_available_cb)(UINT16 nbyte, void* ctx);

/**
 * @brief Opens the connection to a UART device.
 *
 * @param[in] device_id Set to 0 or 1, depending on which UART you want to use
 * @param[in] baud_rate The baud rate of the device
 * @param[in] timeout_ms The timeout in milliseconds to use for both TX and RX
 * @param[in] cb A callback to notify when there is data in the RX buffer.
 *     Leave it NULL if the notification is not needed
 * @param[in] ctx The context to be used in the callback
 *
 * @return `TRUE` if the UART open succeeded, `FALSE` otherwise.
 *
 * @see azx_uart_data_available_cb
 * @see azx_uart_read
 * @see azx_uart_write
 * @see azx_uart_close
 */
BOOLEAN azx_uart_open(UINT16 device_id, UINT32 baud_rate, UINT16 timeout_ms,
    azx_uart_data_available_cb cb, void* ctx);

/**
 * @brief Reads some data from UART.
 *
 * @param[in] device_id The device to read from (0 or 1)
 * @param[out] buffer where the data is stored
 * @param[in] nbyte The maximum number of bytes to read
 *
 * @return The number of byte that have been read, or a negative value in case of error.
 *
 * @see azx_uart_open
 * @see azx_uart_write
 * @see azx_uart_data_available_cb
 */
INT32 azx_uart_read(UINT16 device_id, void* buffer, SIZE_T nbyte);

/**
 * @brief Writes some data to UART.
 *
 * @param[in] device_id The device to write to (0 or 1)
 * @param[in] buffer to write out
 * @param[in] nbyte The number of bytes to write
 *
 * @return The number of byte that have been written, or a negative value in case of error.
 *
 * @see azx_uart_open
 * @see azx_uart_read
 */
INT32 azx_uart_write(UINT16 device_id, const void* buffer,
    SIZE_T nbyte);

/**
 * @brief Closes the connection to a UART device.
 *
 * @param[in] device_id The device to close (0 or 1)
 *
 * @see azx_uart_open
 */
void azx_uart_close(UINT16 device_id);

#endif /* LIBS_HDR_UART_H_ */
