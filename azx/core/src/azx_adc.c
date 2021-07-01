/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "azx_string.h"

#include "azx_ati.h"

#include "azx_adc.h"


UINT16 azx_adc_get(UINT8 adc)
{
  INT32 value = -1;
  
  azx_ati_disable_logs();
  const CHAR* at_rsp = azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT,
			"AT#ADC=%u,2", adc);
      
  azx_ati_deinit();
  
  if(!at_rsp ||
      1 > azx_parseStringf(at_rsp, "%*[^#]#ADC: %d", &value) ||
      value < 0 || value >= 0xFFFF)
  {
    return AZX_ADC_READ_ERROR;
  }
  return (UINT16)value;
}
