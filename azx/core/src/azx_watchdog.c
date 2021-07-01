/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include "m2mb_types.h"
#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"

#include "azx_watchdog.h"

static INT32 watchdogTaskId = -1;
static volatile int watchdog_triggered = 1;

static INT32 watchdogTask(INT32 type, INT32 param1, INT32 param2)
{
  (void)type;
  (void)param1;
  (void)param2;

  while(1)
  {
    azx_sleep_ms(AZX_WATCHDOG_TIMEOUT_MS);
    if(!watchdog_triggered)
    {
      azx_reboot_now();
    }
    watchdog_triggered = 0;
  }
  return 0;
}

void azx_start_watchdog(void)
{
  watchdog_triggered = 1;
  if (0 < (watchdogTaskId = azx_tasks_createTask(
      (char*) "watchdogTask", AZX_TASKS_STACK_XL, 2, AZX_TASKS_MBOX_M, watchdogTask)))
  {
    AZX_LOG_TRACE("Watchdog task ID: %d.\r\n", watchdogTaskId);
    azx_tasks_sendMessageToTask( watchdogTaskId, 0, 0, 0 );
  }
}

void azx_signal_watchdog(void)
{
  watchdog_triggered = 1;
}
