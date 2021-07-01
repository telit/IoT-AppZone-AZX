/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <stdio.h>
#include <string.h>
#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_uart.h"

#include "azx_log.h"

#include "azx_uart.h"

#define INVALID_UART_FD -1

#define MAX_UART_CHANNELS 2

typedef struct
{
  BOOLEAN used;
  INT32 fd;
  azx_uart_data_available_cb data_available_cb;
  void* ctx;
} UartData;

static UartData uartData[MAX_UART_CHANNELS] = {0};

static void m2mb_uart_cb(INT32 fd, M2MB_UART_IND_E uart_event,
    UINT16 resp_size, void* resp_struct,
    void* ctx)
{
  UartData* data = (UartData*)ctx;
  UINT16 size = 0;

  if(!data || uart_event != M2MB_UART_RX_EV || data->fd != fd)
  {
    return;
  }

  AZX_LOG_TRACE("Received UART RX event with size %u (struct=%p)\r\n", resp_size, resp_struct);

  if(resp_size < 2 || !resp_struct)
  {
    return;
  }

  size = *((const UINT16*)resp_struct);
  AZX_LOG_TRACE("We have %u bytes available on UART\r\n", size);
  if(data->data_available_cb != NULL)
  {
    data->data_available_cb(size, data->ctx);
  }
}

static BOOLEAN configure_uart(UartData* data, UINT32 baud_rate, UINT16 timeout_ms)
{
  M2MB_UART_CFG_T cfg;
  if(m2mb_uart_ioctl(data->fd, M2MB_UART_IOCTL_GET_CFG, &cfg) == -1)
  {
    return FALSE;
  }

  cfg.baud_rate = baud_rate;
  // cfg.bits_per_char = M2MB_UART_8_BITS_PER_CHAR;
  // cfg.flow_control = M2MB_UART_FCTL_OFF;
  // cfg.loopback_mode = FALSE;
  // cfg.parity_mode = M2MB_UART_NO_PARITY;
  cfg.tx_timeout_ms = timeout_ms;
  // cfg.stop_bits = M2MB_UART_1_0_STOP_BITS;
  cfg.rx_timeout_ms = timeout_ms;
  if(data->data_available_cb)
  {
    cfg.cb_data = data;
    cfg.cb_fn = m2mb_uart_cb;
  }
  else
  {
    cfg.cb_data = NULL;
    cfg.cb_fn = NULL;
  }

  if(m2mb_uart_ioctl(data->fd, M2MB_UART_IOCTL_SET_CFG, &cfg) == -1)
  {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN azx_uart_open(UINT16 device_id, UINT32 baud_rate, UINT16 timeout_ms,
    azx_uart_data_available_cb cb, void* ctx)
{
  UartData* data = &uartData[device_id];
  CHAR path[16] = { 0 };

  if(device_id > 1)
  {
    AZX_LOG_ERROR("Invalid UART device ID: %u\r\n", device_id);
    return FALSE;
  }

  if(data->used)
  {
    goto end;
  }

  snprintf(path, sizeof(path), "/dev/tty%d", device_id);

  if(INVALID_UART_FD == (data->fd = m2mb_uart_open((const CHAR*)path, 0)))
  {
    AZX_LOG_ERROR("Unable to open UART\r\n");
    goto end;
  }

  data->used = TRUE;
  data->data_available_cb = cb;
  data->ctx = ctx;

  if(!configure_uart(data, baud_rate, timeout_ms))
  {
    AZX_LOG_ERROR("Unable to configure UART\r\n");
    azx_uart_close(device_id);
  }

end:
  return (data->used);
}


INT32 azx_uart_read(UINT16 device_id, void* buffer, SIZE_T nbyte)
{
  UartData* data = &uartData[device_id];

  if(device_id > 1)
  {
    AZX_LOG_ERROR("Invalid UART device ID: %u\r\n", device_id);
    return -1;
  }

  if(!data->used)
  {
    return -1;
  }

  memset(buffer, 0, nbyte);
  return m2mb_uart_read(data->fd, buffer, nbyte);
}

INT32 azx_uart_write(UINT16 device_id, const void* buffer,
    SIZE_T nbyte)
{
  UartData* data = &uartData[device_id];

  if(device_id > 1)
  {
    AZX_LOG_ERROR("Invalid UART device ID: %u\r\n", device_id);
    return -1;
  }

  if(!data->used)
  {
    return -1;
  }

  return m2mb_uart_write(data->fd, buffer, nbyte);
}

void azx_uart_close(UINT16 device_id)
{
  UartData* data = &uartData[device_id];
  if(data->used)
  {
    m2mb_uart_close(data->fd);
    memset(data, 0, sizeof(UartData));
    data->fd = INVALID_UART_FD;
    data->used = FALSE;
  }
}
