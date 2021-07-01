/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_fs_posix.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_apn.h"

/* Ensure the iccid.dat file is stored in the same folder 
  as the app binary in the module filesystem (for example /mod/ )*/

void M2MB_main( int argc, char **argv )
{
    (void)argc;
    (void)argv;
    
    struct ApnInfo *info;

    m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

    AZX_LOG_INIT();
    
    AZX_LOG_INFO("Azx-apn demo. Trying to load parameters from iccid.dat file.\r\n");
    azx_apn_autoSet();
      
    AZX_LOG_INFO("Reading retrieved info...\r\n");
    info = (struct ApnInfo *)azx_apn_getInfo();

    AZX_LOG_INFO("FOR CID %d, CCID: <%s>, APN: <%s>, USER: <%s>, PASS: <%s>, AUTH TYPE: <%d>\r\n", 
        AZX_PDP_CID,
        info->ccid, info->apn, info->username, 
        info->password, info->auth_type);

}
