/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */


#ifndef UUID_9d89065f_c48a_491b_8a63_b575c7fac795
#define UUID_9d89065f_c48a_491b_8a63_b575c7fac795
/**
 * @file azx_watchdog.h
 * @dependencies core/azx_log core/azx_utils core/azx_tasks
 * @version 1.0.1
 * @author Ioannis Demetriou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Software watchdog to detects stalling tasks
 *
 * You can use this watchdog to detect in software if the application is
 * stalling in some task. This is useful if some task processes something that
 * doesn't end.
 *
 * Just call azx_start_watchdog() to start it and signal to it that everything
 * is OK when checkpoints are reached by calling azx_signal_watchdog().
 *
 * To adjust he watchdog timeout, modify @ref AZX_WATCHDOG_TIMEOUT_MS.
 */
#include "m2mb_types.h"
#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"

/**
 * @brief The maximum value between two signals.
 *
 * If the interval in milliseconds elapses without a call to
 * azx_signal_watchdog(), then the modem will reset.
 */
#define AZX_WATCHDOG_TIMEOUT_MS 40000

/**
 * @brief Starts the watchdog to monitor for any stalling threads.
 *
 * This should be called only once. Once the watchdog is started it does not
 * stop, unless a reset happens.
 *
 * @see azx_signal_watchdog
 */
void azx_start_watchdog(void);

/**
 * @brief Signals to the watchdog that everything is OK.
 *
 * This must be called at least once before the expiry of the interval
 * @ref AZX_WATCHDOG_TIMEOUT_MS.
 *
 * @see azx_start_watchdog
 */
void azx_signal_watchdog(void);

#endif /* !defined (UUID_9d89065f_c48a_491b_8a63_b575c7fac795) */
