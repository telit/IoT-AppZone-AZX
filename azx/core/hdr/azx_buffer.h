/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef BUFFER_QUEUE_H
#define BUFFER_QUEUE_H
/**
 * @file azx_buffer.h
 * @version 1.0.1
 * @dependencies core/azx_log
 * @author Sorin Basca
 * @date 10/02/2017
 *
 * @brief Buffers data that can be retrieved later
 *
 * This is a utility library that makes it easy to buffer some data to be
 * passed to another task. Most of the time, if something needs to be handled
 * in another task, there is no convenient way to pass the data
 * (mostly strings). These functions help with that.
 *
 * So typically if you have to pass a string to another class, you can do
 * something like:
 *
 *     INT32 data_idx = azx_buffer_add(new_msg, -1);
 *     azx_tasks_sendMessageToTask(task_id, PARSE_RESPONSE, data_idx, 0);
 *
 * then, in the task you have:
 *
 *     static INT32 my_task(INT32 type, INT32 param1, INT32 param2)
 *     {
 *       switch(type)
 *       {
 *       ...
 *         case PARSE_RESPONSE:
 *         {
 *           const CHAR* response = azx_buffer_get(param1);
 *           parse(response);
 *         }
 *         ...
 *       }
 *     }
 *
 * There is no need to clear the data once consumed, but there is a risk of the
 * data not being reliable. If for example too much data is passed like this
 * too quickly while it's not being processed fast enough, it may be the case
 * that new data will overwrite the old one.
 *
 * Therefore it is important that the design of the flow is done to allow for
 * lower insert rates than consume rates. Other times the errors can be avoided
 * by increasing the inner buffer.
 */
#include "m2mb_types.h"
#include "azx_log.h"

/**
 * @brief Stores a string in the buffer to be retrieved asynchronously
 *
 * @param[in] msg The string to be stored
 * @param[in] len How many bytes of the string to be stored. If the whole string
 *     (until the NUL terminator) is to be stored, set this to -1
 *
 * @return An ID that is used to retrieve the string later. If the string cannot
 *     be stored, this will be -1.
 *
 * @see azx_buffer_get
 */
INT32 azx_buffer_add(const CHAR* msg, INT32 len);

/**
 * @brief Reads the string that has been stored
 *
 * @param[in] index The ID where the data was stored, as returned by
 *     azx_buffer_add()
 *
 * @return The NUL-terminated string that was stored. If only the partial
 *     string was stored, the library will add the NUL termination. If
 *     anything is wrong (like the ID is -1), the string will be empty.
 *
 * @see azx_buffer_add
 */
const CHAR* azx_buffer_get(INT32 index);

/**
 * @brief Stores a data structure in the buffer to be retrieved asynchronously
 *
 * @param[in] obj The data to be copied and stored
 * @param[in] len How many bytes the structure occupies.
 *
 * @return An ID that is used to retrieve the data later. If the data cannot be
 *     stored, this will be -1.
 *
 * @see azx_buffer_getObj
 * @see AZX_BUFFER_ADD
 */
INT32 azx_buffer_addObj(const void* obj, INT32 len);

/**
 * @brief Reads some data that has been stored
 *
 * @param[in] index The ID where the data was stored, as returned by
 *     azx_buffer_addObj()
 *
 * @return The data that was stored. If anything is wrong (like the ID is -1),
 *     then NULL will be returned.
 *
 * @see azx_buffer_add
 * @see AZX_BUFFER_GET
 */
const void* azx_buffer_getObj(INT32 index);

/**
 * @name Convenience wrappers for copying and reading objects from the buffer
 * @{
 */

/**
 * Gets an object from the buffer. Just pass in the type and index and the
 * object will be retrieved and cast to the right type
 */
#define AZX_BUFFER_GET(type, idx) (const type*)azx_buffer_getObj(idx)
/**
 * Adds an object to the buffer. Just pass in the type and the object, and its
 * size will be retrieved and passed to the copy function.
 */
#define AZX_BUFFER_ADD(type, obj) azx_buffer_addObj(obj, sizeof(type))
/** @} */

#endif /* !defined(BUFFER_QUEUE_H) */
