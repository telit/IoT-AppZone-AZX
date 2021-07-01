#include <stdio.h>
#include <string.h>
#include "stdarg.h"

#include "m2mb_types.h"
#include "m2mb_net.h"
#include "m2mb_pdp.h"
#include "m2mb_os_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os.h"

#include "azx_https.h"


#define _CID 3



INT8 cbEvt = 0;
int CB_COUNT = 0;

INT32 DATA_CB(void *buffer, UINT32 size, INT8* flag)
{
  /**** Stop http donwload on a specific user event
  CB_COUNT++;
  if(CB_COUNT >= 3)
  {
    AZX_LOG_INFO("\r\nCB COUNT LIMIT REACHED\r\n");
   *flag = 1;
    return 0;
  }
   */
  strcat((char*) buffer,"\0");
  MYLOG("%s",buffer);

  return 0;
}

static int http_debug_hk(AZX_HTTP_LOG_HOOK_LEVELS_E level, const char *function, const char *file, int line, const char *fmt, ...)
{
  char buf[512];
  int bufSize = sizeof(buf);
  va_list arg;
  INT32   offset = 0;
  UINT32  now = (UINT32) ( m2mb_os_getSysTicks() * m2mb_os_getSysTickDuration_ms() );

  memset(buf,0,bufSize);

  switch(level)
  {
  case AZX_HTTP_LOG_ERROR:
    offset = sprintf(buf, "%5u.%03u %6s %32s:%-4d -  ",
        now / 1000, now % 1000,
        "ERR - ",
        function,
        line
    );
    break;
  case AZX_HTTP_LOG_INFO:
    offset = 0;
    break;
  case AZX_HTTP_LOG_DEBUG:
    offset = sprintf(buf, "%5u.%03u %6s %32s:%-4d - ",
        now / 1000, now % 1000,
        "DBG - ",
        function,
        line
    );

    break;
  }
  va_start(arg, fmt);
  vsnprintf(buf + offset, bufSize-offset, fmt, arg);
  va_end(arg);

  return MYLOG(buf);

}

void M2MB_main( int argc, char **argv )
{
  int ret;
  AZX_HTTP_OPTIONS opts;
  AZX_HTTP_INFO hi;
  AZX_HTTP_SSL  tls;
  azx_httpCallbackOptions cbOpt;

  /*Initialize PDP context*/

  opts.logFunc = http_debug_hk;
  opts.loglevel = AZX_HTTP_LOG_INFO;
  opts.cid = _CID;
  azx_http_initialize(&opts);


  cbOpt.user_cb_bytes_size = 1500;  //call data callback every 1500 bytes
  cbOpt.cbFunc = DATA_CB;
  cbOpt.cbData = m2mb_os_malloc(cbOpt.user_cb_bytes_size + 1); //one more element for \0
  cbOpt.cbEvtFlag = &cbEvt;

  azx_http_setCB(&hi,cbOpt);

  if(-1 == (ret = azx_http_SSLInit(&tls,M2MB_SSL_NO_AUTH)))
  {
    MYLOG("SSL init error \r\n");
  }

  hi.tls = tls;


  /******* GET *****/
  MYLOG("GET request \r\n");
  //ret = azx_http_get(&hi,(char*) "https://www.google.com");    						  //HTTPS & CHUNKED
  //ret = azx_http_get(&hi,"https://modules.telit.com", response, sizeof(response));    // HTTPS + SSL server auth
  ret = azx_http_get(&hi,(char*) "http://linux-ip.net");                              // HTTP only
  //hi.request.auth_type = azx_AuthSchemaBasic;
  //ret = azx_http_get(&hi,"http://guest:guest@jigsaw.w3.org/HTTP/Basic/");  			  //BASIC AUTH


  /******* HEAD *****/
  //MYLOG("HEAD request \r\n");
  //ret = azx_http_head(&hi,(char*) "https://www.google.com");

  /* RESET
    	memset(&hi, 0, sizeof(HTTP_INFO));
    	memset(response, 0, sizeof(response));
      hi.tls = tls;
   */

  /******* POST ****
    	MYLOG("POST request \r\n");
    	char* data = "This is expected to be sent";
    	hi.request.post_data = (char *)m2mb_os_malloc(sizeof(char)* (strlen(data) + 1));
    	memset(hi.request.post_data, 0, sizeof(sizeof(char)* (strlen(data) + 1)));
    	strcpy(hi.request.post_data,data);
    	ret = azx_http_post(&hi,(char *)"http://postman-echo.com:80/post");
   */
}
