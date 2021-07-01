#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "azx_tasks.h"

#include "app_cfg.h"

INT32 myTaskCB(INT32 type, INT32 param1, INT32 param2)
{
    
    AZX_LOG_INFO("task here!");
    AZX_LOG_INFO("type: %d\r\n", type);  //this will be 1
    AZX_LOG_INFO("param1: %d\r\n", param1);  //this will be 2
    AZX_LOG_INFO("param2: %d\r\n", param2);  //this will be 3
}



void M2MB_main( int argc, char **argv )
{
    INT32 task_id;
    azx_tasks_init();
    
    task_id = azx_tasks_createTask((char*) "mytask_name", AZX_TASKS_STACK_M, 4 /*Priority*/, AZX_TASKS_MBOX_M, myTaskCB);

    if (task_id <= 0)
    {
        AZX_LOG_INFO("cannot create socket task!\r\n");
        return;
    }
    else
    {
        azx_tasks_sendMessageToTask(task_id, 1,2,3);
    }
    
    //... do something
    
    azx_tasks_destroyTask(task_id);
}