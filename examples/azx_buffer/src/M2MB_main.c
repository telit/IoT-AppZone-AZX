/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_fs_posix.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_buffer.h"


typedef struct
{
  INT32 param1;
  char param2[10];
} MY_TEST_STRUCT;

void string_consumer(INT32 buf_id)
{
  AZX_LOG_INFO("\r\nRetrieving buffer %d content..\r\n", buf_id);
  const CHAR *data = azx_buffer_get(buf_id);
  if (0 == strlen(data))
  {
    AZX_LOG_WARN("no data retrieved!\r\n");
  }
  else
  {
    AZX_LOG_INFO("data: <%s>\r\n\r\n", data);
  }
}


void struct_consumer(INT32 buf_id)
{
  AZX_LOG_INFO("\r\nRetrieving struct buffer %d content..\r\n", buf_id);
  const MY_TEST_STRUCT *data = AZX_BUFFER_GET(MY_TEST_STRUCT, buf_id);
  if (!data)
  {
    AZX_LOG_WARN("no data retrieved!\r\n");
  }
  else
  {
    AZX_LOG_INFO("data: {%d, <%s>}\r\n\r\n", data->param1, data->param2);
  }
}

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;

  INT32 id;
  MY_TEST_STRUCT test;

  AZX_LOG_INIT();

  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INFO("Azx-buffer demo.\r\n\r\n");


  id = azx_buffer_add("test string", -1);
  if(id != -1)
  {
    AZX_LOG_INFO("Calling consumer to use data on buffer id: %d\r\n", id);
    string_consumer(id);
  }
  else
  {
    AZX_LOG_ERROR("Could not allocate buffer!\r\n");
  }

  test.param1 = 10;
  strcpy(test.param2, "hello-world");

  id = AZX_BUFFER_ADD(MY_TEST_STRUCT, &test);
  if(id != -1)
  {
    AZX_LOG_INFO("Calling consumer to use data on buffer %d\r\n", id);
    struct_consumer(id);
  }
  else
  {
    AZX_LOG_ERROR("Could not allocate structure buffer!\r\n");
  }
}
