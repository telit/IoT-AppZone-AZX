#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "azx_log.h"

#include "app_cfg.h"

#include "azx_ftp.h"


/* Local defines =======================================================================*/
#define DLFILE "Pattern.txt"
#define DLTOBUF_FILE "Pattern.txt"

#define FTP_ADDR "ftp.telit.com"

struct REMFILE {
    struct REMFILE *next;
    fsz_t fsz;
    char *fnm;
};

static UINT32 get_uptime(void)
{

  UINT32 sysTicks = m2mb_os_getSysTicks();

  FLOAT32 ms_per_tick = m2mb_os_getSysTickDuration_ms();

  return (UINT32) (sysTicks * ms_per_tick); //milliseconds
}

static const char* get_file_title(const CHAR* path)
{
  const CHAR* p = path;

  while (*p) {
    if (*p == '/' || *p == '\\') {
      return p + 1;
    }

    p++;
  }
  return path;
}

static int log_progress(AZX_FTP_NET_BUF_T *ctl, azx_ftp_fsz_t xfered, void *arg)
{
  (void) ctl;
  struct REMFILE *f = (struct REMFILE *) arg;
  if ( f->fsz )
  {
    double pct = (xfered * 100.0) / f->fsz;
    AZX_LOG_INFO("%s %5.2f%% %u\r\n", f->fnm, pct, xfered);
  }
  else
  {
    AZX_LOG_INFO("%s %u\r\rn", f->fnm, xfered);
  }

  return 1;
}


static INT32 ftp_debug_hk(AZX_FTP_DEBUG_HOOK_LEVELS_E level, const CHAR *function,
    const CHAR *file, INT32 line, const CHAR *fmt, ...)
{
  char buf[512];
  int bufSize = sizeof(buf);
  va_list arg;
  INT32   offset = 0;
  UINT32  now = get_uptime();

  memset(buf,0,bufSize);

  switch(level)
  {
  case AZX_FTP_DEBUG_HOOK_ERROR:
    offset = sprintf(buf, "%5u.%03u %6s - %15s:%-4d - %32s - ",
        now / 1000, now % 1000,
        "ERR - ",
        get_file_title(file), line,
        function);
    break;
    break;
  case AZX_FTP_DEBUG_HOOK_INFO:
    offset = 0;
    break;
  case AZX_FTP_DEBUG_HOOK_DEBUG:
    offset = sprintf(buf, "%5u.%03u %6s - %15s:%-4d - %32s - ",
        now / 1000, now % 1000,
        "DBG - ",
        get_file_title(file), line,
        function);
    break;
  default:
    offset = 0;
    break;
  }
  va_start(arg, fmt);
  vsnprintf(buf + offset, bufSize-offset, fmt, arg);
  va_end(arg);

  return AZX_LOG_INFO(buf);
}


static int log_progress(netbuf *ctl, fsz_t xfered, void *arg)
{
    struct REMFILE *f = (struct REMFILE *) arg;
    if ( f->fsz )
    {
    double pct = (xfered * 100.0) / f->fsz;
    AZX_LOG_INFO("%s %5.2f%% %u\r\n", f->fnm, pct, xfered);
    }
    else
    {
      AZX_LOG_INFO("%s %u\r\rn", f->fnm, xfered);
    }

    return 1;
}


INT32 my_data_cb(CHAR *data, UINT32 datalen, INT32 ev)
{
    switch(ev)
    {
        case DATA_CB_START:
            //do some initialization before receiving data (e.g. open a file descriptor)
        break;
        case DATA_CB_DATA:
          //manage the data
        break;
        case DATA_CB_END:
            //finalize
        break;
    }
  return 1;
}

AZX_FTP_NET_BUF_T *ftp_client = NULL;
struct REMFILE f;
    

void M2MB_main( int argc, char **argv )
{
    AZX_FTP_OPTIONS_T opts;
    AZX_FTP_CALLBACK_OPTIONS_T cb_opts;

    
    int ret;
    UINT32 file_size;
    char tmp[128];
    
    char FTP_USER[] = "myuser";
    char FTP_PASS[] = "mypass";
    
    /*
    ....
        Check registration and enable PDP context on CID 3
    ....        
    */
    
    opts.level = AZX_FTP_DEBUG_HOOK_ERROR;
    opts.cbFunc = ftp_debug_hk;
    opts.cid = 3; //use CID 3
    
    ret = azx_ftp_init(&opts);
    if (ret != 1)
    {
        MYLOG("failed initializing ftp_client... \r\n");
        return;
    }
    ret = azx_ftp_connect(FTP_ADDR, &ftp_client);
    if (ret == 1)
    {
        MYLOG("Connected.\r\n");
    }
    else
    {
        MYLOG("failed connecting.. \r\n");
        return;
    }    
    ret = azx_ftp_login(FTP_USER, FTP_PASS, ftp_client);
    if (ret == 1)
    {
        MYLOG("FTP login successful.\r\n");
    }
    else
    {
        MYLOG("failed login.. \r\n");
        return;
    }
    
    f.fnm = (char*)DLFILE;
  f.fsz = 0;
    
    MYLOG("Get remote file %s size\r\n", DLFILE);
    ret = azx_ftp_size(f.fnm, &file_size, AZX_FTP_BINARY, ftp_client);
    if (ret == 1)
    {
        MYLOG("Done. File size: %u.\r\n", file_size);
        f.fsz = file_size;        
    }
    else
    {
        MYLOG("failed file size.. error: %s \r\n", azx_ftp_lastResponse(ftp_client));
        return;
    }
    
    MYLOG("Get remote file %s last modification date\r\n", (char*)DLFILE);
    ret = azx_ftp_modDate((char*)DLFILE, tmp, sizeof(tmp), ftp_client);
    if (ret == 1)
    {
        MYLOG("Done. File last mod date: %s.\r\n", tmp);
    }
    else
    {
        MYLOG("failed file last mod date.. \r\n");
        return;
    }
    
    /*Download to file*/
    {
        AZX_FTP_XFER_T local;
        azx_ftp_clearCallback(ftp_client);
        
        memset(&local, 0, sizeof(AZX_FTP_XFER_T));
        
        cb_opts.cbFunc = log_progress;
        cb_opts.cbArg = &f;    /* Pass the file name and size as parameter */
        cb_opts.idleTime = 1000;      /* Call the callback function every second if there were no data exchange */
        cb_opts.bytesXferred = 1024;  /* Call the callback function every 1024 Bytes data exchange */

        local.type = AZX_FTP_XFER_FILE;   //define local type as file: it will be stored in filesystem
        local.payload.fileInfo.path = (char*) "/mod/" DLFILE;  //define the local file path

        azx_ftp_setCallback(&opt,ftp_client);
        
        MYLOG("starting download of remote file %s\r\n", DLFILE);

        ret = azx_ftp_get(&(local), DLFILE, FTPLIB_BINARY, ftp_client);
        if (ret == 1)
        {
            MYLOG("download successful.\r\n");
        }
        else
        {
            MYLOG("failed download.. \r\n");
            return;
        }
        azx_ftp_clearCallback(ftp_client);
    }
    
    /*Upload from file*/
    {
        AZX_FTP_XFER_T local;
        memset(&local, 0, sizeof(AZX_FTP_XFER_T));

        azx_ftp_clearCallback(ftp_client);
        
        f.fnm = (char*) "/mod/" DLFILE;
        f.fsz = 0;
        struct M2MB_STAT st;
        
        if (0 ==m2mb_fs_stat(f.fnm, &st)) //Retrieve local file information (size)
        {
            MYLOG("local file %s size: %u\r\n", f.fnm,  st.st_size);
            f.fsz = st.st_size;
        }
        else
        {
            MYLOG("cannot get local file %s size.\r\n", f.fnm);
            return;
        }

        opt.cbFunc = log_progress;
        opt.cbArg = &f;
        opt.idleTime = 1000;
        opt.bytesXferred = 1024;
        FtpSetCallback(&opt,ftp_client);

        local.type = AZX_FTP_XFER_FILE;          //define the local recipient as file
        local.payload.fileInfo.path = f.fnm;     //define the local file path


        MYLOG("starting upload of local file %s\r\n", local.payload.fileInfo.path);
        ret = azx_ftp_put(&(local), "m2mb_test.txt", AZX_FTP_BINARY, ftp_client);

        if (ret == 1)
        {
            MYLOG("Upload successful.\r\n");
        }
        else
        {
            MYLOG("Upload failed...\r\n");
            return;
        }
        azx_ftp_clearCallback(ftp_client);
    }

    /*Download file to buffer*/
    {
        AZX_FTP_XFER_T local;
        memset(&local, 0, sizeof(AZX_FTP_XFER_T));
        
        f.fnm = (char*)DLTOBUF_FILE;
        
        MYLOG("Get remote file %s size\r\n", f.fnm);
        ret = azx_ftp_size(f.fnm, &file_size, AZX_FTP_BINARY, ftp_client);
        if (ret == 1)
        {
            MYLOG("Done. File size: %u.\r\n", file_size);
        }
        else
        {
            MYLOG("failed file size.. error: %s \r\n", azx_ftp_lastResponse(ftp_client));
            return;
        }

        azx_ftp_clearCallback(ftp_client);
        opt.cbFunc = log_progress;
        opt.cbArg = &f;
        opt.idleTime = 1000;
        opt.bytesXferred = 1024;

        local.type = AZX_FTP_XFER_BUFF;    //set local recipient type as buffer
        local.payload.buffInfo.buffer = (char*) m2mb_os_malloc(file_size + 2);  //pass the buffer to be used to hold file content
        local.payload.buffInfo.bufferSize = file_size + 1;          //pass buffer size
        local.payload.buffInfo.buf_cb = my_data_cb; //the data callback needed to manage the data.
        
        memset(local.payload.buffInfo.buffer,0,local.payload.buffInfo.bufferSize + 2);


        azx_ftp_setCallback(&opt,ftp_client);
        
        MYLOG("starting download of remote file %s to buffer\r\n", DLTOBUF_FILE);

        ret = azx_ftp_get(&(local), DLTOBUF_FILE, FTPLIB_BINARY, ftp_client);

        if (ret > 0)
        {
            int i = 0;
            MYLOG("download successful. Received %d bytes<<<\r\n", ret);

            while (i < ret)
            {
                int toprint = (ret - i < 1024)? ret - i: 1024;
                MYLOG("%.*s", toprint, local.payload.buffInfo.buffer + i);
                i +=toprint;
            }
            MYLOG(">>>\r\n");
        }
        else
        {
            MYLOG("Download failed...\r\n");
            m2mb_os_free(local.payload.buffInfo.buffer);
            return;
        }
        m2mb_os_free(local.payload.buffInfo.buffer);        
    }
    
  MYLOG("FTP quit...\r\n");
  azx_ftp_quit(ftp_client);
}