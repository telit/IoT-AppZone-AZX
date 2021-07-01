/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_713c3323_6a68_43dd_80ac_e7dddf0013e8
#define UUID_713c3323_6a68_43dd_80ac_e7dddf0013e8
/**
 * @file azx_ati.h
 * @version 1.0.2
 * @dependencies core/azx_buffer core/azx_log core/azx_utils
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Sending AT commands and handling URCs
 *
 * The `azx_ati` library provides an easy API for sending AT commands and
 * monitoring Unsolicited Responses (URCs).
 *
 * It can operate over 2 instances and there is no setup necessary for
 * initializing the instance. If you care which AT instance is used, just use
 * the *_ex version of the functions. Otherwise there are macros defined which
 * automatically go to instance 0, so you don't need to specify it in each call
 *
 * All the azx_ati_sendCommand() functions are synchronous. They only return
 * once a response has been received, or the timeout triggered.
 *
 * To receive unsolicited responses, use azx_ati_addUrcHandler(). If you want
 * some unsolicited message ignored (so it does not risk leaking into an
 * AT response), then set the URC handler for it to @ref azx_urc_noop_cb.
 *
 * Each AT instance will have its own URC handlers, so if you use multiple
 * instances, make sure to register handlers on both as appropriate.
 */
#include "m2mb_types.h"
#include "azx_log.h"
#include "azx_buffer.h"
#include "azx_utils.h"

/**
 * @brief If unsure about what timeout to use, you can use this default value.
 */
#define AZX_ATI_DEFAULT_TIMEOUT 2000

/**
 * @brief Binary data structure
 *
 * This is what is used to return from azx_ati_sendCommandBinaryEx()
 */
struct AzxBinaryData
{
  /** @brief Size of the binary data stored */
  SSIZE_T size;
  /** @brief The binary data itself */
  const UINT8* data;
};

/**
 * @name Convenience macros that default to using AT instance 0
 * @{
 */
#define azx_ati_sendCommand(...) azx_ati_sendCommandEx(0, __VA_ARGS__) /**< @see azx_ati_sendCommandEx */
#define azx_ati_sendCommandExpectOk(...) azx_ati_sendCommandExpectOkEx(0, __VA_ARGS__) /**< @see azx_ati_sendCommandExpectOkEx */
#define azx_ati_sendCommandBinary(...) azx_ati_sendCommandBinaryEx(0, __VA_ARGS__) /**< @see azx_ati_sendCommandBinaryEx */
#define azx_ati_sendCommandAndLog(...) azx_ati_sendCommandAndLogEx(0, __VA_ARGS__) /**< @see azx_ati_sendCommandAndLogEx */
#define azx_ati_addUrcHandler(...) azx_ati_addUrcHandlerEx(0, __VA_ARGS__) /**< @see azx_ati_addUrcHandlerEx */
#define azx_ati_deinit(...) azx_ati_deinitEx(0) /**< @see azx_ati_deinitEx */
/** @} */

/**
 * @brief Sends an AT command and returns its response.
 *
 * If there is no response by the timeout, NULL will be returned.
 *
 * The result is valid until a new call to any azx_ati_sendCommand() on the
 * same instance.
 *
 * Example:
 *
 *     // Reads a certain GPIO pin
 *     const CHAR* result = azx_ati_sendCommandEx(0, AZX_ATI_DEFAULT_TIMEOUT, "AT#GPIO=%d,0,0", pin);
 *     if(result == NULL)
 *     {
 *       // Log timeout
 *     }
 *     else
 *     {
 *       // Parse result to check the value of the pin
 *     }
 *
 * @param[in] instance The AT instance to use (0 or 1).
 * @param[in] timeout_ms How long to wait for a response
 * @param[in] cmd The command format followed by any parameters to resolve it.
 *
 * @return The response string. It is valid until the next call to any
 *     azx_ati_sendCommand() on the same instance. If there was no
 *     response before timeout, `NULL` is returned.
 *
 * @see azx_ati_sendCommandExpectOkEx
 * @see azx_ati_sendCommandBinaryEx
 */
const CHAR* azx_ati_sendCommandEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd, ...);

/**
 * @brief Sends an AT command and checks that OK is received.
 *
 * Example:
 *
 *     // Sets a certain GPIO pin to HIGH
 *     if( ! azx_ati_sendCommandExpectOkEx(0, AZX_ATI_DEFAULT_TIMEOUT, "AT#GPIO=%d,1,1", pin) )
 *     {
 *       // Log failure
 *     }
 *
 * @param[in] instance The AT instance to use (0 or 1).
 * @param[in] timeout_ms How long to wait for a response
 * @param[in] cmd The command format followed by any parameters to resolve it.
 *
 * @return One of
 *     True if OK was received,
 *     False if a timeout, or error occurred.
 *
 * @see azx_ati_sendCommandEx
 * @see azx_ati_sendCommandBinaryEx
 */
BOOLEAN azx_ati_sendCommandExpectOkEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd, ...);

/**
 * @brief Sends an AT command and returns its binary response.
 *
 * If there is no response by the timeout, NULL will be returned.
 *
 * The result is valid until a new call to any azx_ati_sendCommand() on the
 * same instance.
 *
 * This is useful if an AT command is expected to return binary data. Since the
 * data can contain string terminating characters, this function must be used.
 *
 * Example:
 *
 *     // Reads a certain GPIO pin
 *     const struct AzxBinaryData* result;
 *     result = azx_ati_sendCommandBinaryEx(0, AZX_ATI_DEFAULT_TIMEOUT, "AT#HTTPRCV=0,%d", size);
 *     if(result == NULL)
 *     {
 *       // Log timeout
 *     }
 *     else
 *     {
 *       // Parse result to read the data as needed
 *     }
 *
 * @param[in] instance The AT instance to use (0 or 1).
 * @param[in] timeout_ms How long to wait for a response
 * @param[in] cmd The command format followed by any parameters to resolve it.
 *
 * @return The response data. It is valid until the next call to any
 *     azx_ati_sendCommand() on the same instance. If there was no response
 *     before timeout, `NULL` is returned.
 *
 * @see azx_ati_sendCommandEx
 * @see azx_ati_sendCommandExpectOkEx
 */
const struct AzxBinaryData* azx_ati_sendCommandBinaryEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd, ...);

/**
 * @brief Runs an AT command and logs the command and response.
 *
 * This is the same as azx_ati_sendCommandEx, except it logs the command even though the component
 * is set to be silent. It can be used to log some parameters that can aid debugging.
 *
 * @param[in] instance The AT instance to use (0 or 1).
 * @param[in] timeout_ms How long to wait for a response
 * @param[in] cmd The command format followed by any parameters to resolve it.
 *
 * @return The response string. It is valid until the next call to any
 *     azx_ati_sendCommand() on the same instance. If there was no
 *     response before timeout, `NULL` is returned.
 *
 * @see azx_ati_sendCommandEx
 * @see azx_ati_disable_logs
 */
const CHAR* azx_ati_sendCommandAndLogEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd, ...);

/**
 * @brief Callback notifying of a new URC.
 *
 * This is the type of the callback that is sent when an URC is received.
 *
 * To receive the callback, the function must be registered through
 * azx_ati_addUrcHandlerEx().
 *
 * @param[in] msg The URC message received. It's always a single line.
 *
 * @see azx_ati_addUrcHandlerEx
 * @see azx_urc_noop_cb
 */
typedef void (*azx_urc_received_cb)(const CHAR* msg);

/**
 * @brief Adds a new URC handler to the instance.
 *
 * Each handler is stored per instance, so if using multiple instances you need
 * to cover both of them.
 *
 * Example:
 *
 *     // Get notified of SIM events
 *     void sim_event_cb(const CHAR* msg)
 *     {
 *       // Parse the "#QSS: %d" string
 *     }
 *
 *     azx_ati_addUrcHandlerEx(0, "#QSS:", &sim_event_cb);
 *
 * @param[in] instance The AT instance to use (0 or 1)
 * @param[in] msg_header How the URC message is expected to look
 * @param[in] cb The function to handle that message
 *
 * @see azx_urc_received_cb
 * @see azx_urc_noop_cb
 */
void azx_ati_addUrcHandlerEx(UINT8 instance, const CHAR* msg_header, azx_urc_received_cb cb);

/**
 * @brief Convenience callback which makes some URC be ignored.
 *
 * Sometimes the modem may issue some URC that we don't want to handle, but we
 * don't want it by mistake to make it into the AT response buffer. For example
 * a "#RING" can occur while we are waiting for an AT response. To tidily handle
 * that just issue:
 *
 *     azx_ati_addUrcHandlerEx("#RING:", &azx_urc_noop_cb);
 *
 * @param[in] msg The message which will be ignored
 *
 * @see azx_urc_received_cb
 * @see azx_ati_addUrcHandlerEx
 */
void azx_urc_noop_cb(const CHAR* msg);

/**
 * @brief Prevents the AZX ATI component from logging a certain AT command command.
 *
 * Some AT commands run very often and they are not relevant for the logs. So, in order to avoid
 * seeing lots of logs that are not needed, some AT commands can be prevented from logging.
 *
 * Errors and warning will still be logged. However, AT command response timeout will be hidden.
 *
 * @param prefix The AT command prefix for which to prevent logging. For example to prevent logging
 * for AT#GPIO set commands, just set the prefix to "AT#GPIO=".
 */
void azx_ati_disable_log_for_cmd(const CHAR* prefix);

/**
 * @brief Disables the logging of all AT commands.
 *
 * This can be used in order to avoid having the AT commands and their responses logged. If the
 * option is used, the only AT commands logged from this component are the ones requested through
 * azx_ati_sendCommandAndLogEx and any errors when azx_ati_sendCommandExpectOkEx is called.
 *
 * This disables the logs for all instances.
 */
void azx_ati_disable_logs();

/**
 * @brief Deinitializes an AT instance.
 *
 * If an ATI instance is no longer needed, this will close all the underlying resources. If that
 * instance will be used again, it will be automatically reinitialized.
 *
 * @param instance The instance to close down.
 */
void azx_ati_deinitEx(UINT8 instance);

#endif /* !defined( UUID_713c3323_6a68_43dd_80ac_e7dddf0013e8 ) */
