#include "m2mb_types.h"
#include "m2mb_sms.h"


#include "azx_log.h"
#include "azx_pduDec.h"

#include "app_cfg.h"

void M2MB_main( int argc, char **argv )
{

  uint8_t pdu[300];
  uint8_t pdu_provv[300];
  int pdulen = azx_pdu_encode((CHAR*)"+321234567890", (CHAR*)"hello world", pdu_provv, 0);
  
  /* pdulen will be changed after the pdu is converted into a binary stream */
  pdulen = azx_pdu_convertZeroPaddedHexIntoByte(pdu_provv, pdu, pdulen);

  //now pdu is a binary PDU that can be sent using m2mb_sms APIs

}