/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"

#include "azx_uart.h"


void uart_data_available_cb(UINT16 nbyte, void* ctx)
{
  INT32 task_id = *(INT32 *) ctx;
  if (task_id > 0)
  {
    azx_tasks_sendMessageToTask(task_id, 0, (INT32) nbyte, 0);
  }

}

INT32 uartTaskCB(INT32 device, INT32 nbyte, INT32 unused)
{
  (void)unused;
  INT32 read;
  UINT8 *buffer = NULL;
  buffer = (UINT8*) m2mb_os_malloc(sizeof(UINT8) * nbyte + 1);
  if (! buffer)
  {
    AZX_LOG_ERROR("Cannot allocate data buffer!!\r\n");
    return -1;
  }
  memset(buffer, 0, nbyte + 1);
  AZX_LOG_INFO("Reading data from UART...\r\n");
  read = azx_uart_read(device, buffer, nbyte);
  AZX_LOG_INFO("Received %d bytes: <%.*s>\r\n", read, read, buffer);

  m2mb_os_free(buffer);
  return 0;
}

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;
  INT32 task_id;
  azx_tasks_init();
  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INIT();

  AZX_LOG_INFO("Azx-uart demo.\r\n");

  task_id = azx_tasks_createTask((char*) "uartTask", AZX_TASKS_STACK_XL, 5, AZX_TASKS_MBOX_M, uartTaskCB);

  if(FALSE == azx_uart_open(0 /*Main UART*/, 115200, 100 /*ms timeout*/, uart_data_available_cb, &task_id))
  {
    AZX_LOG_ERROR("Cannot open UART!!\r\n");
    return;
  }
  else
  {
    azx_uart_write(0, "Ready", 5);
  }

  /* Wait for data input */
  while(1)
  {
    azx_sleep_ms(1000);
  }
}
