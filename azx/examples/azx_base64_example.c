#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os.h"

#include "azx_base64.h"
    

void M2MB_main( int argc, char **argv )
{
    char mystr[] = "helloworld";
    char base64[256];
    char decoded[256];
    int len;
    
    azx_base64Encoder( (UINT8 *) base64, (const UINT8 *) mystr, strlen(mystr));
    
    MYLOG("hash: %s\r\n", base64);
    
    
    len = azx_base64Decoder(decoded, base64);
    
    MYLOG("decoded string: %.*s\r\n", len, decoded);
}
