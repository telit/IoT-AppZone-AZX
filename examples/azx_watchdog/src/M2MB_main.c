/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"
#include "azx_tasks.h"
#include "azx_utils.h"

#include "azx_watchdog.h"

#define KICK_DELAY 5000

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;
  int i;
  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  azx_tasks_init();
  AZX_LOG_INIT();

  AZX_LOG_INFO("Azx-watchdog demo. \r\n");

  AZX_LOG_INFO("Start the watchdog, it will reboot the device if not triggered in 40 seconds\r\n");
  azx_start_watchdog();

  for(i=1; i<=16; i++)
  {
    azx_sleep_ms(KICK_DELAY);
    AZX_LOG_INFO("Kicking watchdog, seconds elapsed: %d\r\n", i * (KICK_DELAY/1000) );
    azx_signal_watchdog();
  }

  AZX_LOG_INFO("Now let the watchdog trigger and reboot the module.\r\n");
  for(i=1; i<=100; i++)
  {
    azx_sleep_ms(KICK_DELAY);
    AZX_LOG_INFO("Seconds elapsed: %d\r\n", i * (KICK_DELAY/1000) );
  }

}
