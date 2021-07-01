/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"
#include "azx_string.h"


void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;

  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INIT();

  AZX_LOG_INFO("Azx-string demo. \r\n");


  {
    CHAR at_rsp[] = "+COPS: 0,0,\"Vodafone\",8";
    INT8 mode = 0xFF;
    CHAR  operator_name[32] = {0};
    INT8 act = 0xFF;

    const CHAR* cops_fmt = "%*[^+]+COPS: %d,%*[^,],\"%32[^\"]\",%d";

    AZX_LOG_INFO("Parsing a +COPS AT command response...\r\n");

    if ( 1 > azx_parseStringf(at_rsp, cops_fmt, &mode, operator_name, &act))
    {
        AZX_LOG_WARN("Unexpected +COPS response\r\n");
    }
    else
    {
        AZX_LOG_INFO("Operator: <%s>, act: %d, mode: %d\r\n\r\n", operator_name, act, mode);
    }
  }

  {
    CHAR at_rsp[] = "#SGACT: 1,1\r\n"
                    "#SGACT: 2,0\r\n"
                    "#SGACT: 3,0\r\n"
                    "#SGACT: 4,0\r\n"
                    "#SGACT: 5,0\r\n"
                    "#SGACT: 6,0\r\n";

    const CHAR* sgact_fmt = "%*[^#]#SGACT: %*[^,],%d%*[^#]#SGACT: %*[^,],%d";

    INT32 cid1 = 0xFF;
    INT32 cid2 = 0xFF;

    AZX_LOG_INFO("Parsing a #SGACT AT command response...\r\n");

    if (1 > azx_parseStringf(at_rsp, sgact_fmt, &cid1, &cid2))
    {
        AZX_LOG_WARN("Unexpected +SGACT response\r\n");
    }
    else
    {
        AZX_LOG_INFO("CID 1 status: %d, CID 2 status: %d\r\n\r\n",  cid1, cid2);
    }

  }

  {
    INT32 ok;
    INT32 number;
    CHAR buffer[] = "String with a number: 45";
    number = azx_getFirstNumber(buffer, &ok);
    if(ok)
    {
      AZX_LOG_INFO("Extracted number: %d\r\n", number);
    }
    else
    {
      AZX_LOG_ERROR("Cannot extract number from string!\r\n");
    }
  }

  {
    const CHAR *list[]= {"the quick brown fox jumped over the lazy dog", "needle in the haystack" };
    const CHAR searchString[] = "needle in the haystack";

    if (TRUE == azx_containsString(list, searchString))
    {
      AZX_LOG_INFO("\r\nString  \"%s\" is in the list\r\n", searchString);
    }
    else
    {
      AZX_LOG_ERROR("\r\nSearch string \"%s\" not found in list!\r\n", searchString);
    }

  }

}
