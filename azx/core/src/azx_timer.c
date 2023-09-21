/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include "m2mb_types.h"
#include "m2mb_hwTmr.h"
#include "azx_log.h"
#include "azx_tasks.h"
#include "azx_utils.h"

#include "azx_timer.h"

typedef struct {
  INT32 id;
  INT32 task_id;
  INT32 type;
  M2MB_HWTMR_HANDLE hnd;
  UINT32 duration_ms;
  azx_expiration_cb cb;
  void* ctx;
} Timer;

#define MAX_TIMERS 10

static Timer allTimers[MAX_TIMERS] = {0};

static INT32 timerTaskId = -1;


static Timer* get_next_available(void);
static BOOLEAN are_timers_unused(void);
static void close_timer(Timer* tim);
static BOOLEAN is_timer_running(Timer* tim);
static Timer* do_init(Timer* tim, UINT32 duration_ms);
static void do_deinit(Timer* tim);
static Timer* get_timer(AZX_TIMER_ID id);

static INT32 timer_task(INT32 unused_type, INT32 timer_id, INT32 unused_param);
static BOOLEAN create_internal_task_if_needed(void);
static BOOLEAN destroy_internal_task_if_not_needed(void);

static void do_start(Timer* tim, UINT32 duration_ms, BOOLEAN restart);
static void do_stop(Timer* tim);

static UINT32 time_now(void);


static Timer* get_next_available(void)
{
  UINT32 i;
  for(i = 0; i < MAX_TIMERS; ++i)
  {
    if(allTimers[i].id == NO_AZX_TIMER_ID)
    {
      allTimers[i].id = i+1;
      return &allTimers[i];
    }
  }
  return 0;
}

static BOOLEAN are_timers_unused(void)
{
  UINT32 i;
  for(i = 0; i < MAX_TIMERS; ++i)
  {
    if(allTimers[i].id != NO_AZX_TIMER_ID)
    {
      return FALSE;
    }
  }
  return TRUE;
}

static void timer_cb(M2MB_HWTMR_HANDLE handle, void *arg)
{
  Timer* tim = (Timer*)arg;
  if(!arg)
  {
    AZX_LOG_WARN("NULL timer info in callback\r\n");
    return;
  }
  AZX_LOG_TRACE("Timer %d expired\r\n", tim->id);
  if(tim->hnd != handle)
  {
    AZX_LOG_WARN("Timer handle %p different from callback handle %p\r\n", tim->hnd, handle);
    return;
  }
  if(tim->id == 0 || tim->id > MAX_TIMERS || &allTimers[tim->id-1] != tim)
  {
    AZX_LOG_WARN("Timer ID %d doesn't match timer structure in callback\n", tim->id);
    return;
  }
  AZX_LOG_TRACE("Sending message to task %d\r\n", tim->task_id);
  azx_tasks_sendMessageToTask( tim->task_id, tim->type, tim->id, 0 );
}

static void close_timer(Timer* tim)
{
  if(tim)
  {
    if(tim->hnd)
    {
      m2mb_hwTmr_deinit(tim->hnd);
      tim->hnd = 0;
    }
    tim->id = NO_AZX_TIMER_ID;
  }
}

static BOOLEAN is_timer_running(Timer* tim)
{
  UINT32 state = 0xDEADBEEF;
  M2MB_HWTMR_RESULT_E res;

  if(!tim || !tim->hnd)
  {
    return FALSE;
  }

  res = m2mb_hwTmr_getItem( tim->hnd, M2MB_HWTMR_SEL_CMD_STATE, &state, NULL );
  if( res != M2MB_HWTMR_SUCCESS )
  {
    AZX_LOG_WARN("Unable to read the state of timer %d\r\n", tim->id);
    return FALSE;
  }
  if( state != M2MB_HWTMR_STATE_RUN  )
  {
    return FALSE;
  }
  return TRUE;
}

static Timer* do_init(Timer* tim, UINT32 duration_ms)
{
  M2MB_HWTMR_RESULT_E res;
  M2MB_HWTMR_ATTR_HANDLE attr;

  res = m2mb_hwTmr_setAttrItem(&attr, 1, M2MB_HWTMR_SEL_CMD_CREATE_ATTR, NULL);
  if(res != M2MB_HWTMR_SUCCESS)
  {
    AZX_LOG_ERROR("Failed to init hwTmr attributes. Error %d\r\n", res);
    goto err;
  }

  tim->duration_ms = (duration_ms > 0 ? duration_ms : 1000);
  res = m2mb_hwTmr_setAttrItem( &attr,
      CMDS_ARGS(
        M2MB_HWTMR_SEL_CMD_CB_FUNC, &timer_cb,
        M2MB_HWTMR_SEL_CMD_ARG_CB, tim,
        M2MB_HWTMR_SEL_CMD_TIME_DURATION, M2MB_HWTMR_TIME_MS(tim->duration_ms),
        M2MB_HWTMR_SEL_CMD_PERIODIC, M2MB_HWTMR_ONESHOT_TMR,
        M2MB_HWTMR_SEL_CMD_AUTOSTART, M2MB_HWTMR_NOT_START
        )
      );
  if( res != M2MB_HWTMR_SUCCESS )
  {
    AZX_LOG_ERROR("Failed to init hwTmr attributes. Error %d\r\n", res);
    goto err;
  }

  res = m2mb_hwTmr_init(&tim->hnd, &attr);

  if( res != M2MB_HWTMR_SUCCESS )
  {
    AZX_LOG_ERROR("Failed to init hwTmr. Error %d\r\n", res);
    m2mb_hwTmr_setAttrItem( &attr, 1, M2MB_HWTMR_SEL_CMD_DEL_ATTR, NULL );
    goto err;
  }

  if(is_timer_running(tim))
  {
    AZX_LOG_TRACE("Timer is initialized in running mode\r\n");
  }
  else
  {
    AZX_LOG_TRACE("Timer is initialized in idle mode\r\n");
  }

  return tim;

err:
  close_timer(tim);
  return 0;
}

static void do_deinit(Timer* tim)
{
  if(is_timer_running(tim))
  {
    AZX_LOG_TRACE("Stopping timer before deinit\r\n");
    do_stop(tim);
  }

  close_timer(tim);
  return;
}


AZX_TIMER_ID azx_timer_init(INT32 task_id, INT32 type, UINT32 duration_ms)
{
  Timer* tim = get_next_available();
  if(!tim)
  {
    AZX_LOG_DEBUG("Unable to add a new timer, no more space\r\n");
    goto err;
  }

  tim = do_init(tim, duration_ms);
  if(!tim)
  {
    AZX_LOG_DEBUG("Unable to initialise a new timer\r\n");
    goto err;
  }

  tim->task_id = task_id;
  tim->type = type;
  AZX_LOG_TRACE("Initialized timer with ID %d successfully\r\n", tim->id);
  return tim->id;

err:
  close_timer(tim);
  return NO_AZX_TIMER_ID;
}

BOOLEAN azx_timer_deinit(AZX_TIMER_ID timer_id)
{
  Timer* tim = get_timer(timer_id);
  if(!tim)
  {
    AZX_LOG_DEBUG("Timer is not initialized\r\n");
    goto err;
  }

  do_deinit(tim);

  destroy_internal_task_if_not_needed();
  return TRUE;

err:
  destroy_internal_task_if_not_needed();
  return FALSE;
}

static Timer* get_timer(AZX_TIMER_ID id)
{
  Timer* tim;
  if(id == 0 || id > MAX_TIMERS)
  {
    AZX_LOG_WARN("Timer ID %d out of bounds\r\n", id);
    return 0;
  }

  tim = &allTimers[id-1];

  if(tim->id != id)
  {
    AZX_LOG_WARN("Timer ID %d not matching %d\r\n", id, tim->id);
    return 0;
  }

  return tim;
}

static INT32 timer_task(INT32 unused_type, INT32 timer_id, INT32 unused_param)
{
  (void)unused_type;
  (void)unused_param;
  const Timer* timer = get_timer(timer_id);

  if(timer && timer->cb)
  {
    timer->cb(timer->ctx, timer_id);
  }
  return 0;
}

static BOOLEAN create_internal_task_if_needed(void)
{
  if (0 < timerTaskId)
  {
    return TRUE;
  }

  if(0 < (timerTaskId = azx_tasks_createTask(
          (CHAR*) "Timer",
          AZX_TASKS_STACK_L, 2, AZX_TASKS_MBOX_L,
          timer_task)))
  {
    return TRUE;
  }

  AZX_LOG_ERROR("Unable to create timer helper task\r\n");
  return FALSE;
}

static BOOLEAN destroy_internal_task_if_not_needed(void)
{
  BOOLEAN res = FALSE;
  if (0 < timerTaskId)
  {
    return TRUE;
  }

  if(are_timers_unused())
  {
    AZX_LOG_DEBUG("Destroy timer task\r\n");

    if(0 < azx_tasks_destroyTask(timerTaskId))
    {
      res =  TRUE;
    }
    else
    {
      AZX_LOG_ERROR("Unable to destroy timer helper task\r\n");
    }
  }
  else
  {
    /*do nothing*/
  }
  return res;
}

AZX_TIMER_ID azx_timer_initWithCb(azx_expiration_cb cb, void* ctx, UINT32 duration_ms)
{
  AZX_TIMER_ID id;
  Timer* timer = 0;
  if(!create_internal_task_if_needed())
  {
    goto error;
  }

  id = azx_timer_init(timerTaskId, 0, duration_ms);
  if(id == NO_AZX_TIMER_ID || 0 == (timer = get_timer(id)))
  {
    goto error;
  }

  timer->cb = cb;
  timer->ctx = ctx;

  return id;
error:
  close_timer(timer);
  return NO_AZX_TIMER_ID;
}



void azx_timer_start(AZX_TIMER_ID id, UINT32 duration_ms, BOOLEAN restart)
{
  Timer* tim = get_timer(id);

  if(!tim)
  {
    AZX_LOG_WARN("Cannot start timer\r\n");
    return;
  }

  do_start(tim, duration_ms, restart);
}

static void do_start(Timer* tim, UINT32 duration_ms, BOOLEAN restart)
{
  M2MB_HWTMR_RESULT_E res = M2MB_HWTMR_SUCCESS;

  if(is_timer_running(tim))
  {
    if(!restart)
    {
      AZX_LOG_TRACE("Timer %d already running, won't restart\r\n", tim->id);
      return;
    }
    AZX_LOG_TRACE("Restarting timer %d\r\n", tim->id);
    do_stop(tim);
  }
  else
  {
    AZX_LOG_TRACE("Timer %d idle\r\n", tim->id);
  }

  if(duration_ms != tim->duration_ms && duration_ms > 0)
  {
    UINT32 timeDuration = M2MB_HWTMR_TIME_MS(duration_ms);
    tim->duration_ms = duration_ms;
    res = m2mb_hwTmr_setItem(tim->hnd, M2MB_HWTMR_SEL_CMD_TIME_DURATION, (void*)timeDuration);
    if(res != M2MB_HWTMR_SUCCESS)
    {
      AZX_LOG_ERROR("Failed to set timer duration. Error %d\r\n", res);
      return;
    }
    else
    {
      AZX_LOG_TRACE("Have set the duration\r\n");
    }
  }

  res = m2mb_hwTmr_start(tim->hnd);
  if(res != M2MB_HWTMR_SUCCESS)
  {
    AZX_LOG_ERROR("Failed to start timer. Error %d\r\n", res);
    return;
  }
  AZX_LOG_TRACE("Timer %d started\r\n", tim->id);
}

void azx_timer_stop(AZX_TIMER_ID id)
{
  Timer* tim = get_timer(id);

  if(!tim)
  {
    AZX_LOG_WARN("Cannot stop timer\r\n");
    return;
  }

  do_stop(tim);
}

static void do_stop(Timer* tim)
{
  M2MB_HWTMR_RESULT_E res = m2mb_hwTmr_stop(tim->hnd);
  if(res != M2MB_HWTMR_SUCCESS)
  {
    AZX_LOG_WARN("Failed to stop timer. Error %d\r\n", res);
  }
}

static UINT32 time_now(void)
{
   UINT32 sysTicks = m2mb_os_getSysTicks();
   FLOAT32 ms_per_tick = m2mb_os_getSysTickDuration_ms();
   return (UINT32) (sysTicks * ms_per_tick); //milliseconds
}

UINT32 azx_timer_getTimestampFromNow(UINT32 interval_ms)
{
  UINT32 now = time_now();
  // TODO: deal with overflow
  return (now + interval_ms);
}

void azx_timer_waitUntilTimestamp(UINT32 timestamp)
{
  UINT32 now = time_now();
  if(timestamp == 0 || now >= timestamp)
  {
    return;
  }
  while(now < timestamp)
  {
    azx_sleep_ms(timestamp-now);
    now = time_now();
  }
}

BOOLEAN azx_timer_hasTimestampPassed(UINT32 timestamp)
{
  UINT32 now = time_now();
  return (timestamp < now);
}
