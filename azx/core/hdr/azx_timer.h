/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_9669089d_c8c5_4c9f_898b_37377eee8e07
#define UUID_9669089d_c8c5_4c9f_898b_37377eee8e07
/**
 * @file azx_timer.h
 * @version 1.0.4
 * @dependencies core/azx_log core/azx_tasks core/azx_utils
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief A better way to use timers
 *
 * This library provides a convenient way to define and use timers in the
 * application.
 *
 * One can use asynchronous timer with azx_timer_init() to receive notification
 * on an existing task (see azx_tasks.h), or specify a custom callback using
 * azx_timer_initWithCb(). Timers can then be used with azx_timer_start().
 *
 * Furthermore, timestamp related operations can be performed. For example you can
 * make a cool-down timer synchronously using azx_timer_getTimestampFromNow()
 * and the related functions.
 */
#include "m2mb_types.h"
#include "azx_log.h"
#include "azx_tasks.h"
#include "azx_utils.h"

/**
 * @brief The ID of a timer.
 *
 * @see azx_timer_init
 * @see azx_timer_initWithCb
 */
typedef INT32 AZX_TIMER_ID;

/**
 * @brief Callback issued when a timer expires.
 *
 * The callback will be issued on a dedicate task, therefore the handler can do
 * any intense/blocking processing it requires. The only limitation is that
 * other timers using the callback will be pending until the handling is done,
 * but the real-time tasks will not be affected.
 *
 * @param[in] ctx The context that was passed to  azx_timer_initWithCb()
 * @param[in] timer_id The ID of the timer as returned by azx_timer_initWithCb()
 *
 * @see azx_timer_initWithCb
 */
typedef void (*azx_expiration_cb)(void* ctx, AZX_TIMER_ID timer_id);

/**
 * @brief The value that signifies invalid timer ID.
 *
 * @see azx_timer_init
 * @see azx_timer_initWithCb
 */
#define NO_AZX_TIMER_ID 0

/**
 * @brief Initializes a timer.
 *
 * The timer can be started for a certain amount of time. Once it expires a
 * message will be queued to the task provided. Do not expect that the task
 * will handle the expired notification at the exact time, as there may be some
 * delays introduced in handling the queued message. The only guarantee is that
 * at least the minimum duration has passed.
 *
 * The message sent on expiry will have the type as defined here, `param1` will
 * be the ID of the timer and `param2` will be unused.
 *
 * @param[in] task_id The task where the expired message is stored.
 * @param[in] type The type of the message to notify as the expiry
 * @param[in] duration_ms The interval between callbacks. If it doesn't change,
 *     set it to something, otherwise leave it 0 and use the parameter in
 *     azx_timer_start().
 *
 * @return The ID of the created timer, or @ref NO_AZX_TIMER_ID if something failed.
 *
 * @see azx_timer_initWithCb
 * @see azx_timer_start
 */
AZX_TIMER_ID azx_timer_init(INT32 task_id, INT32 type, UINT32 duration_ms);


/**
 * @brief Deinitializes a timer.
 *
 * The timer will be removed and all related resources will be freed.
 *
 * @param[in] timer_id The ID of the timer to be deleted.
 *
 * @return The ID of the created timer, or @ref NO_AZX_TIMER_ID if something failed.
 *
 * @see azx_timer_initWithCb
 * @see azx_timer_init
 */
BOOLEAN azx_timer_deinit(AZX_TIMER_ID timer_id);

/**
 * @brief Initializes a timer that executes a cb upon completion.
 *
 * The timer can be started for a certain amount of time. Once it expires the
 * callback function will be executed.
 *
 * @warning This is running on the timer task. Keep the processing to the min
 * (queue to other tasks)
 *
 * Do not expect that the task will handle the expired notification at the exact
 * time, as there may be some delays introduced in handling the queued message.
 * The only guarantee is that at least the minimum duration has passed.
 *
 * The message sent on expiry will have the type as defined here, `param1` will
 * be the ID of the timer and `param2` will be unused.
 *
 * @param[in] cb The function to call upon timer expiration
 * @param[in] ctx Passes this to the cb function
 * @param[in] duration_ms The interval between callbacks. If it doesn't change,
 *     set it to something, otherwise leave it 0 and use the parameter in
 *     azx_timer_start().
 *
 * @return The ID of the created timer, or @ref NO_AZX_TIMER_ID if something failed.
 *
 * @see azx_timer_init
 * @see azx_timer_start
 */
AZX_TIMER_ID azx_timer_initWithCb(azx_expiration_cb cb, void* ctx, UINT32 duration_ms);

/**
 * @brief Starts a timer.
 *
 * Once the duration elapses a new message will be received.
 *
 * @param[in] id The ID of the timer
 * @param[in] duration_ms The duration in milliseconds. If the duration doesn't
 *     change from last set, just leave it 0.
 * @param[in] restart If set to TRUE then the timer will be restarted if
 *     already running. If set to `FALSE`, then, if the timer is started, it
 *     will be allowed to continue without restart (new request is discarded).
 *
 * @see azx_timer_init
 * @see azx_timer_initWithCb
 * @see azx_timer_stop
 */
void azx_timer_start(AZX_TIMER_ID id, UINT32 duration_ms, BOOLEAN restart);

/**
 * @brief Stops a timer.
 *
 * @param[in] id The ID of the timer
 *
 * @see azx_timer_start
 */
void azx_timer_stop(AZX_TIMER_ID id);

/**
 * @brief Gets a timestamp in the future relative to now.
 *
 * @param[in] interval_ms How many milliseconds in the future to set the timestamp.
 *
 * @return The timestamp
 *
 * @see azx_timer_waitUntilTimestamp
 * @see azx_timer_hasTimestampPassed
 */
UINT32 azx_timer_getTimestampFromNow(UINT32 interval_ms);

/**
 * @brief Blocks waiting for the timestamp to be reached.
 *
 * If the timestamp is in the past, or it's 0, it returns straight away,
 * otherwise the thread will sleep.
 *
 * @param[in] timestamp The timestamp that is needed.
 *
 * @see azx_timer_getTimestampFromNow
 * @see azx_timer_hasTimestampPassed
 */
void azx_timer_waitUntilTimestamp(UINT32 timestamp);

/**
 * @brief Checks if a certain timestamp is in the past, or not.
 *
 * @param[in] timestamp The timestamp to check
 *
 * @return `TRUE` if that timestamp has passed already, `FALSE` otherwise.
 *
 * @see azx_timer_getTimestampFromNow
 * @see azx_timer_waitUntilTimestamp
 */
BOOLEAN azx_timer_hasTimestampPassed(UINT32 timestamp);

#endif /* !defined (UUID_9669089d_c8c5_4c9f_898b_37377eee8e07) */
