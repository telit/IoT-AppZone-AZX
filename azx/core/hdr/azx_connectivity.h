/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_b58c7e49_57a9_4bec_b485_ad7fa0690b25
#define UUID_b58c7e49_57a9_4bec_b485_ad7fa0690b25
/**
 * @file azx_connectivity.h
 * @version 1.0.2
 * @dependencies core/azx_apn core/azx_log core/azx_utils core/azx_timer
 * @author Ioannis Demetriou
 * @author Fabio Pintus
 * @date 10/02/2019
 *
 * @brief Establish network and data connection synchronously and provide info
 *
 * This library easily establishes network connection, both via blocking and
 * non-blocking methods, and can subsequently provide network statistics such
 * as RSSI, BER etc.
 *
 * Furthermore, it provides similar functionality for establishing PDP context.
 * It is assumed that APN has been properly configured in the appropriate CID.
 * The recommended way to do this is with the azx_apn.h methods.
 */
#include "m2mb_types.h"
#include "m2mb_net.h"
#include "app_cfg.h"
#include "azx_log.h"
#include "azx_utils.h"
#include "azx_timer.h"
#include "azx_apn.h"

/**
 * @brief Use to make the thread wait until connectivity is established
 *
 * @see azx_connectivity_ConnectToNetworkSyncronously
 * @see azx_connectivity_ConnectPdpSyncronously
 */
#define TRY_FOREVER 0

/**
 * @brief Synchronously connect to the carrier network, but **NOT** activate PDP
 *
 *
 * @param[in] timeout_sec How much time to wait if a failure occurs while connecting.
 *     If set to @ref TRY_FOREVER, then this will not return until connecting.
 *
 * @return One of:
 *     `TRUE` if device connected to the network,
 *     `FALSE` if timeout has been exhausted.
 */
BOOLEAN azx_connectivity_ConnectToNetworkSyncronously(UINT32 timeout_sec);

/**
 * @brief Synchronously connect to the mobile network and establish a PDP context.
 *
 * @param[in] timeout_sec How much time to wait if a failure occurs while connecting.
 *     If set to @ref TRY_FOREVER, then this will not return until connecting.
 *
 * @return One of:
 *     `TRUE` if device connected to the network and activated PDP,
 *     `FALSE` if timeout has been exhausted.
 *
 * @see azx_connectivity_deactivatePdp
 */
BOOLEAN azx_connectivity_ConnectPdpSyncronously(UINT32 timeout_sec);

/**
 * @brief Get current RSSI value.
 *
 * The value is the same as the one obtained from running AT+CSQ.
 * If there is no connectivity 99 is returned.
 */
INT8 azx_connectivity_getRssi();

/**
 * @brief Get current Radio Access Technology
 */
M2MB_NET_RAT_E azx_connectivity_getRat();

/**
 * @brief Disconnects from the PDP context.
 *
 * @param[in] timeout_sec How much to wait for the disconnect. It is recommended to allow it for at
 *              least 1 minute if you plan to do anything with connectivity afterwards. If not, then
 *              you can make it smaller. If set to @ref TRY_FOREVER, this will not return until
 *              disconnect occurs.
 *
 * @return `TRUE` if deactivation succeeded, `FALSE` otherwise.
 *
 * @see azx_connectivity_ConnectPdpSyncronously
 */
BOOLEAN azx_connectivity_deactivatePdpSyncronously(UINT32 timeout_sec);

/**
 * @brief Structure providing the connectivity info.
 *
 * @see azx_connectivity_getInfo
 */
typedef struct {
  M2MB_NET_STAT_E status; /**< Current status of the connectivity. */
  M2MB_NET_RAT_E rat; /**< The Radio Technology used to communicate with the serving cell. */
  INT8 rssi; /**< The RSSI value in dBm. If not connected, this is 99. */
  INT16 ber; /**< The bit error rate. If not connected, this is 99. */
} AZX_CONNECTIVITY_INFO_T;

/**
 * @brief Retrieves synchronously the current connectivity information
 *
 * This function may wait up to 1 second to retrieve all the information.
 *
 * @return The connectivity information obtained.
 *
 * @see AZX_CONNECTIVITY_INFO_T
 */
const AZX_CONNECTIVITY_INFO_T* azx_connectivity_getInfo();

#endif /* UUID_b58c7e49_57a9_4bec_b485_ad7fa0690b25 */
