/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_fs_posix.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_ati.h"

void creg_event_cb(const CHAR* msg)
{
 AZX_LOG_INFO("received data from CREG command: <%s>\r\n", msg);
}


void M2MB_main( int argc, char **argv )
{
    (void)argc;
    (void)argv;

    m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

    AZX_LOG_INIT();
    
    AZX_LOG_INFO("Azx-ati demo. Default AT instance 0 will be used.\r\n");
    AZX_LOG_INFO("To use a specific instance, please refer to azx_ati_*Ex functions\r\n");
    
    AZX_LOG_INFO("Disable logs for AT#MONI\r\n");
    azx_ati_disable_log_for_cmd("#MONI");

    AZX_LOG_INFO("Registering callback for CREG events\r\n");
    azx_ati_addUrcHandler("+CREG:", &creg_event_cb);

    AZX_LOG_INFO("Enabling URCs for CREG...\r\n");
    if (TRUE != azx_ati_sendCommandExpectOk(AZX_ATI_DEFAULT_TIMEOUT, "AT+CREG=%d\r", 1))
    {
      AZX_LOG_ERROR("cannot send AT command!\r\n");
    }
    
    AZX_LOG_INFO("Sending AT#MONI...\r\n");
    azx_ati_sendCommandExpectOk(AZX_ATI_DEFAULT_TIMEOUT, "AT#MONI\r");
    
    AZX_LOG_INFO("Now wait for registration events from CREG...\r\n");

}
