/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"
#include "azx_timer.h"

void timer_expiration_cb(void* ctx, AZX_TIMER_ID timer_id)
{
  static INT8 count = 0;
  INT32 *context_data = (INT32*) ctx;
  AZX_LOG_INFO("--- In timer callback, received %d context data\r\n", *((INT32*) ctx) );
  count++;
  if(count < 5)
  {
    (*context_data)++;
    AZX_LOG_INFO("--- Restarting the timer...\r\n\r\n");
    azx_timer_start(timer_id, 2000, FALSE);
  }
  else
  {
    AZX_LOG_INFO("5 cycles done, deinit timer.\r\n\r\n");
    azx_timer_deinit(timer_id);
  }
}

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;
  INT32 *mycontextdata = NULL;
  AZX_TIMER_ID id = NO_AZX_TIMER_ID;
  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  azx_tasks_init();

  AZX_LOG_INIT();

  AZX_LOG_INFO("Azx-timer demo. \r\n");


  mycontextdata = (INT32*) m2mb_os_malloc(sizeof(INT32));
  if(!mycontextdata)
  {
    AZX_LOG_ERROR("Cannot allocate context data!!\r\n");
    return;
  }
  *mycontextdata = 45;

  AZX_LOG_INFO("Creating a timer with callback function..\r\n");

  id = azx_timer_initWithCb(timer_expiration_cb, mycontextdata, 0);
  if(NO_AZX_TIMER_ID == id)
  {
    AZX_LOG_ERROR("Timer creation failed!\r\n");
    return;
  }
  else
  {
    AZX_LOG_INFO("Starting the timer..\r\n");
    azx_timer_start(id, 1000, FALSE);
  }

  AZX_LOG_INFO("Get timestamp 20 seconds from now and wait...\r\n\r\n");
  UINT32 timestamp = azx_timer_getTimestampFromNow(20000);
  azx_timer_waitUntilTimestamp(timestamp);

  AZX_LOG_INFO("After 20 seconds...\r\n");


  AZX_LOG_INFO("\r\nChecking if a timestamp has passed...\r\n");
  timestamp = azx_timer_getTimestampFromNow(20000);
  azx_sleep_ms(5000);
  AZX_LOG_INFO("After 5 seconds, check the timestamp expiration\r\n");
  if(azx_timer_hasTimestampPassed(timestamp))
  {
    AZX_LOG_INFO("Timestamp already passed!\r\n");
  }
  else
  {
    AZX_LOG_INFO("Timestamp has not passed yet!\r\n");
  }
}
