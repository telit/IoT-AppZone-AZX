/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os_mtx.h"
#include "m2mb_ati.h"

#include "azx_log.h"
#include "azx_buffer.h"
#include "azx_utils.h"

#include "azx_ati.h"

#define BUFFER_SIZE 2048
#define WAIT_BETWEEN_READS_MS 20
#define MAX_URC_HEADER 32
#define MAX_URC_HANDLERS 20
#define MAX_RESPONSES 10
#define MAX_SILENT_CMDS 20

typedef struct
{
  CHAR hdr[MAX_URC_HEADER];
  azx_urc_received_cb cb;
} UrcHandler;

typedef struct
{
  CHAR prefix[MAX_URC_HEADER];
} SilentCmds;

static SilentCmds silentCmds[MAX_SILENT_CMDS] = { 0 };
static BOOLEAN silentMode = FALSE;
static BOOLEAN fullySilentMode = FALSE;
static BOOLEAN logNext = FALSE;

typedef struct
{
  M2MB_ATI_HANDLE h;
  const INT16 instanceID;
  BOOLEAN initialised;
  M2MB_OS_MTX_HANDLE mtx_hnd;
  BOOLEAN processing;
  UINT16 next_response;
  struct AzxBinaryData rsp[MAX_RESPONSES];
  UrcHandler urc_handlers[MAX_URC_HANDLERS];
  UINT8 readData[BUFFER_SIZE];
  UINT16 readDataIdx;
} AtiData;


static AtiData ati_data[] = {
    {
      .h = NULL,
      .instanceID = 0,
      .initialised = FALSE,
      .mtx_hnd = M2MB_OS_MTX_INVALID,
      .processing = FALSE,
      .next_response = 0,
    },
    {
      .h = NULL,
      .instanceID = 1,
      .initialised = FALSE,
      .mtx_hnd = M2MB_OS_MTX_INVALID,
      .processing = FALSE,
      .next_response = 0,
    },
    {
      .h = NULL,
      .instanceID = 2,
      .initialised = FALSE,
      .mtx_hnd = M2MB_OS_MTX_INVALID,
      .processing = FALSE,
      .next_response = 0,
    }
};

#define MAX_AT_INSTANCES (sizeof(ati_data)/sizeof(AtiData))

static BOOLEAN is_silent_command(const CHAR* cmd);

static AtiData* get_ati_data(UINT8 instance)
{
  if(instance < MAX_AT_INSTANCES)
  {
    return &ati_data[instance];
  }

  AZX_LOG_ERROR("Requested AT instance %u, only %u available\r\n",
      instance, sizeof(ati_data)/sizeof(AtiData));
  return NULL;
}

static UINT8 get_index_of_instance(const AtiData* data)
{
  return data - &ati_data[0];
}

static void lock(AtiData* data)
{
  M2MB_OS_RESULT_E osRes;
  if(M2MB_OS_SUCCESS != (osRes = m2mb_os_mtx_get(data->mtx_hnd, 0xFFFFFFFF)))
  {
    AZX_LOG_WARN("Unable to lock mutex to protect AT instance (err = %d)\r\n", osRes);
  }
}

static void unlock(AtiData* data)
{
  if(!data->mtx_hnd)
  {
    return;
  }
  if(M2MB_OS_SUCCESS != m2mb_os_mtx_put(data->mtx_hnd))
  {
    AZX_LOG_WARN("Unable to unlock mutex to protect AT instance\r\n");
  }
}

static UINT8* find_hdr(SSIZE_T size, UINT8* bytes, const UINT8* hdr)
{
  UINT16 i = 0;
  UINT16 hdr_idx = 0;
  
  if( !bytes )
  {
    return NULL;
  }

  while(i < size && hdr[hdr_idx] != '\0')
  {
    if(bytes[i] == hdr[hdr_idx])
    {
      ++hdr_idx;
      ++i;
      continue;
    }
    /* Start of line doesn't match so go to next line */
    hdr_idx = 0;
    while(i < size && (bytes[i] != '\r' && bytes[i] != '\n'))
    {
      ++i;
    }
    while(i < size && (bytes[i] == '\r' || bytes[i] == '\n'))
    {
      ++i;
    }
  }

  if(hdr[hdr_idx] != '\0')
  {
    return NULL;
  }
  if(!silentMode)
  {
    AZX_LOG_TRACE("Found URC header '%s' (i=%u, hdr_idx=%u)\r\n", hdr, i, hdr_idx);
  }
  return &bytes[i-hdr_idx];
}

static struct AzxBinaryData handle_urc_lines(AtiData* data, SSIZE_T size, UINT8* bytes)
{
  UINT8 i;
  struct AzxBinaryData response = {0};

  if( !bytes )
  {
    return response;
  }

  for(i = 0; i < MAX_URC_HANDLERS && size > 0; ++i)
  {
    UrcHandler* hnd = &data->urc_handlers[i];
    UINT8* p = NULL;
    if(!hnd->cb)
    {
      break;
    }
    
    if(!silentMode)
    {
      AZX_LOG_TRACE("Checking URC with header %s\r\n", hnd->hdr);
    }
    
    while(NULL != (p = find_hdr(size, bytes, (const UINT8*)hnd->hdr)))
    {
      UINT8* end = p;
      UINT8* buf_end = bytes+size;

      if(!silentMode)
      {
        AZX_LOG_TRACE("Finding end of URC\r\n");
      }
      while(end < buf_end && *end != '\r' && *end != '\n')
      {
        ++end;
      }
      if(!silentMode)
      {
        AZX_LOG_TRACE("Found end of URC\r\n");
      }
      if(end[0] == '\r' && end[1] == '\n')
      {
        ++end;
      }
      else if(end[0] == '\0')
      {
        --end;
      }
      else
      {
        *end = '\0';
      }
      if(!silentMode)
      {
        AZX_LOG_TRACE("Cutting out URC\r\n");
      }
      AZX_LOG_DEBUG("Reporting URC: %s\r\n", (const CHAR*)p);
      hnd->cb((const CHAR*)p);
      if(!silentMode)
      {
        AZX_LOG_TRACE("Completely removing out URC (p=%p, end=%p, bytes=%p,size=%d)\r\n", p, end, bytes, size);
      }
      ++end;
      while(end < buf_end)
      {
        *p++ = *end++;
      }
      size -= (end-p);
      if(!silentMode)
      {
        AZX_LOG_TRACE("New size is %d\r\n", size);
      }
      if(size < 0)
      {
        size = 0;
      }
      bytes[size] = '\0';
      if(!silentMode)
      {
        AZX_LOG_TRACE("Done handling URC\r\n");
      }
    }
  }

  while(size > 0  && (*bytes == '\r' || *bytes == '\n'))
  {
    ++bytes;
    --size;
  }

  response.size = size;
  response.data = bytes;

  if(!silentMode)
  {
    AZX_LOG_TRACE("AT response after URC handling: {%.*s}\r\n", size, bytes);
  }

  return response;
}

static const CHAR* const MessageEndings[] = {
  "OK\r",
  "ERROR\r",
  "+CME ERROR:",
  "> ",
  "CONNECT",
  "NO CARRIER",
  NULL
};

static BOOLEAN starts_with(const UINT8* buf, UINT16 size, const CHAR* str)
{
  UINT16 idx = 0;
  while(*str != '\0')
  {
    if(idx == size)
    {
      return FALSE;
    }
    if(buf[idx] != (UINT8)*str)
    {
      return FALSE;
    }
    ++idx;
    ++str;
  }
  return TRUE;
}

static BOOLEAN is_message_complete(const struct AzxBinaryData* msg)
{
  UINT16 idx;
  UINT16 i = 0;
  for(idx = 0; idx < msg->size; ++idx)
  {
    if(!silentMode)
    {
      AZX_LOG_TRACE("Checking completeness from '%.*s'\r\n",
          msg->size - idx, (const CHAR*)&(msg->data[idx]));
    }
    for(i = 0; MessageEndings[i] != NULL; ++i)
    {
      if(!silentMode)
      {
        AZX_LOG_TRACE("Comparing with %s\r\n", MessageEndings[i]);
      }
      if(starts_with(&(msg->data[idx]), msg->size - idx, MessageEndings[i]))
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

static UINT16 process_ati_response(AtiData* data, SSIZE_T size, UINT8* bytes, UINT16 idx)
{
  struct AzxBinaryData response = handle_urc_lines(data, size, &bytes[idx]);

  if(response.size <= 0)
  {
    return idx;
  }


  if(!data->processing)
  {
    if(!silentMode)
    {
      AZX_LOG_TRACE("No outstanding AT command ignoring\r\n");
    }
    idx = 0;
    goto end;
  }

  idx += response.size;
  if(is_message_complete(&response))
  {
    if(!silentMode)
    {
      AZX_LOG_TRACE("AT response is complete (size=%u)\r\n", idx);
    }
    data->rsp[data->next_response].data = (const UINT8*)(
        azx_buffer_getObj(azx_buffer_addObj(bytes, idx)));
    if(data->rsp[data->next_response].data)
    {
      data->rsp[data->next_response].size = idx;
    }
    idx = 0;
  }
  else
  {
    if(!silentMode)
    {
      AZX_LOG_TRACE("Buffering AT response (buffered %u bytes)\r\n", idx);
    }
  }

end:

  return idx;
}

static void ati_cb( M2MB_ATI_HANDLE h, M2MB_ATI_EVENTS_E ati_event,
    UINT16 resp_size, void *resp_struct, void *userdata )
{
  AtiData* data = (AtiData*)userdata;
  UINT16 size = 0;
  SSIZE_T read_size = 0;

  if(!data || ati_event != M2MB_RX_DATA_EVT || data->h != h)
  {
    return;
  }

  if(!silentMode)
  {
    AZX_LOG_TRACE("Received AT RX event with size %u (struct=%p)\r\n", resp_size, resp_struct);
  }

  if(resp_size < 2 || !resp_struct)
  {
    return;
  }

  lock(data);

  size = *((const UINT16*)resp_struct);
  if(!silentMode)
  {
    AZX_LOG_TRACE("Outstanding %u bytes\r\n", size);
  }

  while(size >= BUFFER_SIZE-data->readDataIdx-1)
  {
    if(data->readDataIdx > 0)
    {
      AZX_LOG_WARN("AT response is bigger in size than the buffer size. Dropping old buffer\r\n");
      data->readDataIdx = 0;
    }
    else
    {
      AZX_LOG_WARN("AT response is still bigger in size than the buffer size\r\n");
      unlock(data);
      return;
    }
  }

  if(data->readDataIdx == 0)
  {
    memset(data->readData, 0, sizeof(data->readData));
  }
  if(!silentMode)
  {
    AZX_LOG_TRACE("Receive AT buffer at index %u\r\n", data->readDataIdx);
  }
  read_size = m2mb_ati_rcv_resp(h, (void*)&data->readData[data->readDataIdx], size);
  if(read_size <= 0)
  {
    AZX_LOG_WARN("Unable to perform AT read\r\n");
    unlock(data);
    return;
  }

  data->readData[data->readDataIdx + read_size] = '\0';
  if(!silentMode)
  {
    AZX_LOG_TRACE("Read AT buffer (size=%d): %.*s\r\n", read_size, read_size,
        (const CHAR*)&data->readData[data->readDataIdx]);
  }

  data->readDataIdx = process_ati_response(data, read_size, data->readData, data->readDataIdx);
  unlock(data);

}

__attribute__((weak)) M2MB_RESULT_E ati_send_cmd( M2MB_ATI_HANDLE handle, void *buf, SIZE_T nbyte )
{
  return m2mb_ati_send_cmd(handle, buf, nbyte);
}

__attribute__((weak)) SSIZE_T ati_rcv_resp( M2MB_ATI_HANDLE handle, void *buf, SIZE_T nbyte )
{
  return m2mb_ati_rcv_resp(handle, buf, nbyte);
}

void azx_urc_noop_cb(const CHAR* msg)
{
  AZX_LOG_DEBUG("Ignoring URC: %s\r\n", msg);
  (void)msg;
}

static void send_initial_config_commands(const AtiData* data)
{
  BOOLEAN rc = FALSE;
  UINT8 instance = get_index_of_instance(data);
  AZX_LOG_TRACE("Bringing AT instance %d:%d in a known state\r\n", instance, data->instanceID);
  rc = azx_ati_sendCommandExpectOkEx(instance, AZX_ATI_DEFAULT_TIMEOUT, "ATE0");

  if(!rc)
  {
    AZX_LOG_WARN("ATI (id:%d) configuration commands failed.\r\n",
        data->instanceID);
  }
}

/**
 * Opens ATI instance to be used sending commands.
 *
 * @warning This instance will have echo disabled
 */
static BOOLEAN open_ati_handle(AtiData* data)
{
  if(data->initialised)
  {
    return TRUE;
  }

  AZX_LOG_TRACE("Opening ATI %d \r\n", data->instanceID);

  if(!data->mtx_hnd)
  {
    UINT32 inheritVal = 1;
    M2MB_OS_RESULT_E osRes;
    M2MB_OS_MTX_ATTR_HANDLE mtxAttrHandle;

    osRes = m2mb_os_mtx_setAttrItem_( &mtxAttrHandle,
        M2MB_OS_MTX_SEL_CMD_CREATE_ATTR, NULL,
        M2MB_OS_MTX_SEL_CMD_NAME, "atMtx",
        M2MB_OS_MTX_SEL_CMD_USRNAME, "atMtx",
        M2MB_OS_MTX_SEL_CMD_INHERIT, inheritVal);
    if(osRes != M2MB_OS_SUCCESS)
    {
      AZX_LOG_WARN("Unable to create mutex attr to protect AT instance (err = %d)\r\n", osRes);
      return FALSE;
    }
    osRes = m2mb_os_mtx_init(&data->mtx_hnd, &mtxAttrHandle);
    if(!data->mtx_hnd || osRes != M2MB_OS_SUCCESS)
    {
      AZX_LOG_WARN("Unable to create mutex to protect AT instance (err = %d)\r\n", osRes);
      return FALSE;
    }
    AZX_LOG_TRACE("Created mutex to protect the AT instance\r\n");
  }

  if(M2MB_RESULT_SUCCESS != m2mb_ati_init(&data->h, data->instanceID, &ati_cb, data))
  {
    return FALSE;
  }

  data->initialised = TRUE;
  memset(data->rsp, 0, sizeof(data->rsp));
  data->readDataIdx = 0;

  /* Since the starting state of the AT instance cannot be guaranteed,
   * this function executes several AT instance profile commands to
   * bring it to a known state.*/
  send_initial_config_commands(data);

  return TRUE;
}

static void enable_next_response(AtiData* data, BOOLEAN advance)
{
  if(advance)
  {
    if(++data->next_response == MAX_RESPONSES)
    {
      data->next_response = 0;
    }
  }
  data->processing = FALSE;
}

static const struct AzxBinaryData* wait_for_response(AtiData* data, INT32 timeout_ms)
{
  INT32 retries = (timeout_ms / WAIT_BETWEEN_READS_MS) + 1;
  UINT16 rsp_id = data->next_response;

  for(; retries > 0; --retries)
  {
    lock(data);
    if ( data->rsp[rsp_id].size > 0 )
    {
      if(!silentMode)
      {
        AZX_LOG_DEBUG("Received response: %s\r\n", (const CHAR*)(data->rsp[rsp_id].data));
      }
      if(logNext && silentMode)
      {
        AZX_LOG_INFO("%s\r\n", (const CHAR*)(data->rsp[rsp_id].data));
      }
      enable_next_response(data, TRUE);
      unlock(data);
      return &data->rsp[rsp_id];
    }
    unlock(data);
    azx_sleep_ms(WAIT_BETWEEN_READS_MS);
  }

  if(!silentMode)
  {
    AZX_LOG_ERROR("No AT response received before timeout\r\n");
  }
  if(logNext && silentMode)
  {
    AZX_LOG_DEBUG("<Timed out>");
  }
  enable_next_response(data, FALSE);
  return NULL;
}

static const struct AzxBinaryData* send_at_command_v(UINT8 instance, INT32 timeout_ms,
    const CHAR* cmd_fmt, va_list args)
{
  static CHAR cmd[BUFFER_SIZE];
  INT32 cmd_len = 0;
  AtiData* data = get_ati_data(instance);
  const struct AzxBinaryData* result = 0;

  if(!data)
  {
    AZX_LOG_ERROR("Instance %d is not registered with the library.\r\n");
    return NULL;
  }

  if(!open_ati_handle(data))
  {
    AZX_LOG_ERROR("Unable to open AT handle\r\n");
    return NULL;
  }

  lock(data);
  while(data->processing)
  {
    unlock(data);
    azx_sleep_ms(WAIT_BETWEEN_READS_MS);
    lock(data);
  }
  data->processing = TRUE;

  vsnprintf(cmd, BUFFER_SIZE-2, cmd_fmt, args);

  cmd_len = strlen(cmd);
  cmd[cmd_len] = '\0';
  if(is_silent_command(cmd))
  {
    silentMode = TRUE;
  }
  else
  {
    AZX_LOG_DEBUG("Sending: %s\r\n", cmd);
  }

  cmd[cmd_len++] = '\r';
  cmd[cmd_len] = '\0';

  data->rsp[data->next_response].size = 0;
  data->rsp[data->next_response].data = NULL;
  data->readDataIdx = 0;
  if(M2MB_RESULT_SUCCESS != ati_send_cmd(data->h, cmd, cmd_len))
  {
    AZX_LOG_ERROR("Unable to send command: %s\r\n", cmd);
    enable_next_response(data, FALSE);
    unlock(data);
    result = NULL;
    goto end;
  }
  if(logNext && silentMode)
  {
    AZX_LOG_INFO("%s", cmd);
  }
  unlock(data);

  result = wait_for_response(data, timeout_ms);

end:
  silentMode = FALSE;
  return result;
}

const CHAR* azx_ati_sendCommandEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd_fmt, ...)
{
  const struct AzxBinaryData* response = NULL;
  va_list va;

  va_start(va, cmd_fmt);
  response = send_at_command_v(instance, timeout_ms, cmd_fmt, va);
  va_end(va);

  return (response ? (const CHAR*)response->data : NULL);
}

static BOOLEAN is_response_ok(const struct AzxBinaryData* response)
{
  UINT16 size = 0;
  if(!response)
  {
    return FALSE;
  }

  size = response->size;

  while(size > 0 && (response->data[size-1] == '\r' || response->data[size-1] == '\n'))
  {
    --size;
  }

  if(size >= 2 && response->data[size-2] == 'O' &&
      response->data[size-1] == 'K')
  {
    return TRUE;
  }

  return FALSE;
}

BOOLEAN azx_ati_sendCommandExpectOkEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd_fmt, ...)
{
  const struct AzxBinaryData* response = NULL;
  va_list va;

  va_start(va, cmd_fmt);
  response = send_at_command_v(instance, timeout_ms, cmd_fmt, va);
  va_end(va);

  if(!is_response_ok(response))
  {
    if(fullySilentMode)
    {
      /* Log only in full silent mode to aid debugging. If not in full silent mode and a command was
       * explicitly set to silent, then the outcome is expected to be handled by caller */
      AZX_LOG_DEBUG("Command with format '%s' got unwanted response: %s\r\n",
          cmd_fmt, (response ? (const CHAR*)response->data : "<time out>"));
    }
    return FALSE;
  }
  return TRUE;
}

const struct AzxBinaryData* azx_ati_sendCommandBinaryEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd_fmt, ...)
{
  const struct AzxBinaryData* response = NULL;
  va_list va;

  va_start(va, cmd_fmt);
  response = send_at_command_v(instance, timeout_ms, cmd_fmt, va);
  va_end(va);

  return response;
}

const CHAR* azx_ati_sendCommandAndLogEx(UINT8 instance, INT32 timeout_ms, const CHAR* cmd_fmt, ...)
{
  const struct AzxBinaryData* response = NULL;
  va_list va;

  logNext = TRUE;
  va_start(va, cmd_fmt);
  response = send_at_command_v(instance, timeout_ms, cmd_fmt, va);
  va_end(va);

  logNext = FALSE;

  return (response ? (const CHAR*)response->data : NULL);
}


void azx_ati_addUrcHandlerEx(UINT8 instance, const CHAR* msg_header, azx_urc_received_cb cb)
{
  UrcHandler* hnd = NULL;
  UINT16 i = 0;
  AtiData* data = get_ati_data(instance);

  if(!data)
  {
    AZX_LOG_WARN("Unable to add handler for URC '%s'\r\n", msg_header);
    return;
  }

  for(i = 0; i < MAX_URC_HANDLERS && hnd == NULL; ++i)
  {
    if(data->urc_handlers[i].cb == NULL)
    {
      if(!hnd)
      {
        hnd = &data->urc_handlers[i];
      }
    }
    else if(0 == strcmp(data->urc_handlers[i].hdr, msg_header) &&
        (!hnd || hnd->cb == NULL))
    {
      hnd = &data->urc_handlers[i];
    }
  }

  if(!hnd)
  {
    AZX_LOG_WARN("Unable to add handler for URC '%s'\r\n", msg_header);
    return;
  }

  snprintf(hnd->hdr, MAX_URC_HEADER, "%s", msg_header);
  hnd->cb = cb;

  AZX_LOG_DEBUG("Added handler for URC '%s': %p\r\n", msg_header, cb);
}

void azx_ati_disable_log_for_cmd(const CHAR* prefix)
{
  INT32 i;
  for(i = 0; i < MAX_SILENT_CMDS; ++i)
  {
    if(silentCmds[i].prefix[0] == '\0')
    {
      snprintf(silentCmds[i].prefix, sizeof(silentCmds[i].prefix), "%s", prefix);
      return;
    }
  }
  AZX_LOG_WARN("Cannot make AT prefix %s silent\r\n", prefix);
}

static BOOLEAN is_silent_command(const CHAR* cmd)
{
  INT32 i;
  if(fullySilentMode)
  {
    return TRUE;
  }

  for(i = 0; i < MAX_SILENT_CMDS; ++i)
  {
    if(silentCmds[i].prefix[0] == '\0')
    {
      return FALSE;
    }
    if(strncmp(cmd, silentCmds[i].prefix, strlen(silentCmds[i].prefix)) == 0)
    {
      return TRUE;
    }
  }
  return FALSE;
}

void azx_ati_disable_logs(void)
{
  fullySilentMode = TRUE;
}

void azx_ati_deinitEx(UINT8 instance)
{
  AtiData* data = get_ati_data(instance);
  if(!data)
  {
    return;
  }
  m2mb_ati_deinit(data->h);
  data->h = NULL;
  data->initialised = FALSE;
  m2mb_os_mtx_deinit(data->mtx_hnd);
  data->mtx_hnd = M2MB_OS_MTX_INVALID;
  data->processing = FALSE;
  data->next_response = 0;
}

