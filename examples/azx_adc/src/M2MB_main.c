/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_fs_posix.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_adc.h"



void M2MB_main( int argc, char **argv )
{
    (void)argc;
    (void)argv;
    UINT16 adc_value;
    m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

    AZX_LOG_INIT();
    
    AZX_LOG_INFO("Azx-adc demo. Reading ADC value...\r\n");
    
    adc_value = azx_adc_get(1);
    
    if(AZX_ADC_READ_ERROR != adc_value)
    {
      AZX_LOG_INFO("ADC value is %u\r\n", adc_value);
    }
    else
    {
      AZX_LOG_ERROR("Failed reading ADC value\r\n");
    }

}
