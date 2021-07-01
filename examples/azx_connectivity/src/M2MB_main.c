/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_fs_posix.h"

#include "app_cfg.h"

#include "azx_log.h"
#include "azx_tasks.h"

#include "azx_connectivity.h"
static INT32 main_task(INT32 type, INT32 param1, INT32 param2)
{
  (void)type;
  (void)param1;
  (void)param2;


  AZX_LOG_INFO("Checking network registration (60 seconds of timeout)...\r\n");
  if(TRUE == azx_connectivity_ConnectToNetworkSyncronously(60))
  {
    AZX_LOG_INFO("Device registered.\r\n");
  }
  else
  {
    AZX_LOG_ERROR("Device not registered after timeout!\r\n");
  }

  AZX_LOG_INFO("Current RSSI value: %d\r\n", azx_connectivity_getRssi());

  AZX_LOG_INFO("Current RAT value: %d\r\n", azx_connectivity_getRat());

  AZX_LOG_INFO("Activating PDP context on cid (60 seconds of timeout)...\r\n", AZX_PDP_CID);
  if(TRUE == azx_connectivity_ConnectPdpSyncronously(60))
  {
    AZX_LOG_INFO("PDP context activated. Waiting 10 seconds then disabling it...\r\n");
    m2mb_os_taskSleep(M2MB_OS_MS2TICKS(10000));

    AZX_LOG_INFO("Deactivating PDP context on cid (60 seconds of timeout)...\r\n", AZX_PDP_CID);
    if(TRUE == azx_connectivity_deactivatePdpSyncronously(60))
    {
      AZX_LOG_INFO("PDP context deactivated.\r\n");
    }
    else
    {
      AZX_LOG_ERROR("PDP context not deactive after timeout!\r\n");
    }
  }
  else
  {
    AZX_LOG_ERROR("PDP context not active after timeout!\r\n");
  }

  return 0;
}

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;
  INT32 mainTask = -1;

  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INIT();

  azx_tasks_init();
  mainTask = azx_tasks_createTask((CHAR*)"Main", AZX_TASKS_STACK_XL, 1, AZX_TASKS_MBOX_L,
      main_task);


  AZX_LOG_INFO("Azx-connectivity demo.\r\n");

  azx_tasks_sendMessageToTask(mainTask, 0, 0, 0);




  while(1)
  {
    m2mb_os_taskSleep(M2MB_OS_MS2TICKS(10000));
  }
}
