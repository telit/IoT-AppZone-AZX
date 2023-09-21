/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <string.h>
#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os_mtx.h"

#include "azx_log.h"

#include "azx_buffer.h"

#define MAX_STRING 256
#define MAX_MSG_SIZE MAX_STRING
#define MAX_MSG_COUNT 64
#define BUFFER_SIZE (MAX_MSG_SIZE * MAX_MSG_COUNT)
#define MARKER 0x80000000

static CHAR bufferedMessages[BUFFER_SIZE];
static INT32 nextMessageIndex = 0;

M2MB_OS_MTX_HANDLE mtxHnd = M2MB_OS_MTX_INVALID;

static void lock(void)
{
  M2MB_OS_RESULT_E osRes;
  if(!mtxHnd)
  {
    UINT32 inheritVal = 1;
    M2MB_OS_MTX_ATTR_HANDLE mtxAttrHandle;
    osRes = m2mb_os_mtx_setAttrItem_( &mtxAttrHandle,
        M2MB_OS_MTX_SEL_CMD_CREATE_ATTR, NULL,
        M2MB_OS_MTX_SEL_CMD_NAME, "bufMtx",
        M2MB_OS_MTX_SEL_CMD_USRNAME, "bufMtx",
        M2MB_OS_MTX_SEL_CMD_INHERIT, inheritVal);
    if(osRes != M2MB_OS_SUCCESS)
    {
      AZX_LOG_WARN("Unable to create mutex attr to protect the buffer queue (err = %d)\r\n", osRes);
      return;
    }
    osRes = m2mb_os_mtx_init(&mtxHnd, &mtxAttrHandle);
    if(!mtxHnd || osRes != M2MB_OS_SUCCESS)
    {
      AZX_LOG_WARN("Unable to create mutex to protect the buffer queue (err = %d)\r\n", osRes);
      return;
    }
    AZX_LOG_TRACE("Created mutex to protect the buffer queue\r\n");
  }
  if(M2MB_OS_SUCCESS != (osRes = m2mb_os_mtx_get(mtxHnd, 0xFFFFFFFF)))
  {
    AZX_LOG_WARN("Unable to lock mutex to protect the buffer queue (err = %d)\r\n", osRes);
  }
}

static void unlock(void)
{
  if(!mtxHnd)
  {
    return;
  }
  if(M2MB_OS_SUCCESS != m2mb_os_mtx_put(mtxHnd))
  {
    AZX_LOG_WARN("Unable to unlock mutex to protect the buffer queue\r\n");
  }
}


INT32 azx_buffer_add(const CHAR* msg, INT32 len)
{
  if(len == -1)
  {
    len = strlen(msg);
  }

  return azx_buffer_addObj(msg, len);
}

const CHAR* azx_buffer_get(INT32 idx)
{
  const CHAR* result = (const CHAR*)azx_buffer_getObj(idx);
  if(!result)
  {
    return "";
  }
  return result;
}

INT32 azx_buffer_addObj(const void* obj, INT32 len)
{
  INT32 idx = 0;

  if(len >= BUFFER_SIZE)
  {
    /* Don't buffer messages larger than the whole buffer. */
    return -1;
  }

  lock();
  idx = nextMessageIndex;

  nextMessageIndex += len+1;

  if(nextMessageIndex >= BUFFER_SIZE)
  {
    idx = 0;
    nextMessageIndex = len+1;
  }
  unlock();

  memcpy(&(bufferedMessages[idx]), obj, len);
  bufferedMessages[idx + len] = '\0';
  return idx;
}

const void* azx_buffer_getObj(INT32 idx)
{
  if(idx < 0 || idx >= BUFFER_SIZE)
  {
    return NULL;
  }
  return &(bufferedMessages[idx]);
}

