/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <stdio.h>
#include <string.h>
#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_gpio.h"

#include "azx_log.h"
#include "azx_utils.h"
#include "azx_timer.h"

#include "azx_gpio.h"


#define MAX_GPIO_NUM 10
#define INVALID_GPIO_FD 0


#define ME910_SLEEP_TICKS 30
#define LE910CxL_SLEEP_TICKS 30
#define LE910CxX_SLEEP_TICKS 30
#define ME310_SLEEP_TICKS 15


#define SLEEP_IN_TICKS ME910_SLEEP_TICKS /*default*/

#if AZ_MODULE_FAMILY == 30
#undef SLEEP_IN_TICKS
#define SLEEP_IN_TICKS ME910_SLEEP_TICKS
#endif

#if AZ_MODULE_FAMILY == 37
#undef SLEEP_IN_TICKS
#define SLEEP_IN_TICKS ME310_SLEEP_TICKS
#endif
#if AZ_MODULE_FAMILY == 2520
#undef SLEEP_IN_TICKS
#define SLEEP_IN_TICKS LE910CxL_SLEEP_TICKS
#endif
#if AZ_MODULE_FAMILY == 2530
#undef SLEEP_IN_TICKS
#define SLEEP_IN_TICKS LE910CxX_SLEEP_TICKS
#endif



typedef struct
{
  INT32 fd;
  M2MB_GPIO_DIRECTION_E direction;
  M2MB_GPIO_PULL_MODE_E pull;
} GpioInfo;

/**
 * Array that contains file descriptors for all available GPIOs.
 *
 * Because Telit modem GPIO start counting at 1, it is recommended to use
 * get_gpio_info() function to read the current struct.
 *
 * Furthermore, if the intention is to perform IO operations, it is recommended
 * to use prepare_gpio() that would ensure that the GPIO is already open.
 */
static GpioInfo gpioInfos[MAX_GPIO_NUM] = { 0 };

#define get_index(gpio) (gpio-1) /* Telit GPIO numbering starts at 1 */
/**
 * Use this function to get gpio fd from array above
 *
 * If the intention is to perform IO operations, it is recommended to use
 * prepare_gpio(), that also ensures that the GPIO has been opened.
 */
static GpioInfo* get_gpio_info(UINT8 gpio)
{
  INT32 i = get_index(gpio);
  if(0 <= i && i < MAX_GPIO_NUM)
  {
    return &gpioInfos[i];
  }

  AZX_LOG_WARN("No such GPIO pin\r\n");
  return NULL;
}

typedef struct
{
  UINT8 gpio;
  azx_gpio_interrupt_cb* cb;
  AZX_GPIO_TRIGGER_E trigger;
  UINT32 debounce_until_ts;
  UINT32 cooldown_interval_ms;
} InterruptRequest;

/**
 * Array that contains CBs for interruptRequests
 */
static InterruptRequest interruptRequests[MAX_GPIO_NUM] = { 0 };

/**
 * Use this function to get interrupt info from array above
 */
static InterruptRequest* get_irq(UINT8 gpio)
{
  INT32 i = get_index(gpio);
  if(0 <= i && i < MAX_GPIO_NUM)
  {
    return &interruptRequests[i];
  }

  AZX_LOG_WARN("No such GPIO pin\r\n");
  return NULL;
}

/* Printers {{{ */
const CHAR* azx_gpio_printPull(M2MB_GPIO_PULL_MODE_E pull)
{
  switch(pull)
  {
    case M2MB_GPIO_NO_PULL:
      return "No Pull";
    case M2MB_GPIO_PULL_DOWN:
      return "Pull Down";
    case M2MB_GPIO_PULL_KEEPER:
      return "Same Pull";
    case M2MB_GPIO_PULL_UP:
      return "Pull Up";
    default:
      break;
  }

  return "Unknown Pull";
}

const CHAR* azx_gpio_printTrigger(AZX_GPIO_TRIGGER_E trigger)
{
  switch(trigger)
  {
    case M2MB_GPIO_INTR_POSEDGE:
      return "Rising Edge (low to high)";
    case M2MB_GPIO_INTR_NEGEDGE:
      return "Falling edge (high to low)";
    case M2MB_GPIO_INTR_ANYEDGE:
      return "Dual edge (any change)";
    default:
      break;
  }

  return "Unknown Trigger";
}

const CHAR* azx_gpio_printDirection(M2MB_GPIO_DIRECTION_E direction)
{
  switch(direction)
  {
    case M2MB_GPIO_MODE_INPUT:
      return "Input";
    case M2MB_GPIO_MODE_OUTPUT:
      return "Output";
    default:
      break;
  }

  return "Unknown Direction";
}
/* Printers }}} */

static INT32 open_gpio(UINT8 gpio)
{
  INT32 fd = INVALID_GPIO_FD;
  CHAR path[16];
  snprintf(path, sizeof(path), "/dev/GPIO%u", gpio);
  fd = m2mb_gpio_open((const CHAR*)path, 0);
  if(-1 == fd)
  {
    AZX_LOG_ERROR("Failed to open %s\r\n", path);
    return INVALID_GPIO_FD;
  }

  AZX_LOG_TRACE("GPIO %u opened (fd: %d)\r\n", gpio, fd);
  return fd;
}

static GpioInfo* close_gpio(GpioInfo* gpio_info)
{
  if(!gpio_info)
  {
    return NULL;
  }

  m2mb_gpio_close(gpio_info->fd);
  gpio_info->fd = INVALID_GPIO_FD;

  return gpio_info;
}

#define IS_OPEN(gpio_info) ((gpio_info)->fd != INVALID_GPIO_FD)

static GpioInfo* prepare_gpio(UINT8 gpio)
{
  GpioInfo* info = get_gpio_info(gpio);

  if(!info)
  {
    return NULL;
  }

  if(!IS_OPEN(info))
  {
    info->fd = open_gpio(gpio);
    if(!IS_OPEN(info))
    {
      return NULL;
    }

    info->pull = M2MB_GPIO_PULL_KEEPER;
  }

  return info;
}

static GpioInfo* do_gpio_conf(UINT8 gpio,
    M2MB_GPIO_DIRECTION_E direction, M2MB_GPIO_PULL_MODE_E pull)
{
  GpioInfo* info = prepare_gpio(gpio);
  if(!info)
  {
    return NULL;
  }

  if(info->direction == direction &&
      (pull == M2MB_GPIO_PULL_KEEPER || info->pull == pull))
  {
    return info;
  }

  if(0 != m2mb_gpio_multi_ioctl(info->fd, CMDS_ARGS(
        M2MB_GPIO_IOCTL_SET_DIR, direction,
        M2MB_GPIO_IOCTL_SET_PULL, pull
        )))
  {
    AZX_LOG_WARN("M2MB ioctl request failed\r\n");
    goto error;
  }

  info->direction = direction;
  info->pull = (pull != M2MB_GPIO_PULL_KEEPER) ? pull : info->pull;
  AZX_LOG_DEBUG("GPIO %u: Changed to %s (%s)\r\n", gpio, strgpiodir(direction), strgpiopull(pull));

  return info;

error:
  if(IS_OPEN(info))
  {
    close_gpio(info);
  }
  AZX_LOG_ERROR("Failed to configure GPIO %u as %u (%s)\r\n", gpio, strgpiodir(direction), strgpiopull(pull));
  return NULL;
}

BOOLEAN azx_gpio_conf_ex(UINT8 gpio, M2MB_GPIO_DIRECTION_E direction, M2MB_GPIO_PULL_MODE_E pull)
{
  return (NULL != do_gpio_conf(gpio, direction, pull));
}

BOOLEAN azx_gpio_set(UINT8 gpio, UINT8 status)
{
  GpioInfo* info = do_gpio_conf(gpio, M2MB_GPIO_MODE_OUTPUT, M2MB_GPIO_PULL_KEEPER);
  const M2MB_GPIO_VALUE_E val = (status == AZX_GPIO_LOW ? M2MB_GPIO_LOW_VALUE : M2MB_GPIO_HIGH_VALUE);

  if(!info)
  {
    return FALSE;
  }

  if ( -1 == m2mb_gpio_write( info->fd, val ) )
  {
    AZX_LOG_ERROR("Failed to write GPIO %u with value %u(%d)\r\n", gpio, status, val);
    return FALSE;
  }
  return TRUE;
}

UINT8 azx_gpio_get(UINT8 gpio)
{
  M2MB_GPIO_VALUE_E val;
  GpioInfo* info = do_gpio_conf(gpio, M2MB_GPIO_MODE_INPUT, M2MB_GPIO_PULL_KEEPER);

  if(!info)
  {
    return AZX_GPIO_READ_ERROR;
  }

  if ( -1 == m2mb_gpio_read( info->fd, &val) )
  {
    AZX_LOG_ERROR("Failed to read GPIO %u\r\n", gpio);
    return AZX_GPIO_READ_ERROR;
  }

  return (val == M2MB_GPIO_LOW_VALUE ? AZX_GPIO_LOW : AZX_GPIO_HIGH);
}

BOOLEAN azx_gpio_sendPulse(UINT8 gpio, UINT32 period, UINT32 no_of_waves)
{

  /*Open in input to read initial value*/
  GpioInfo* info = do_gpio_conf(gpio, M2MB_GPIO_MODE_INPUT, M2MB_GPIO_PULL_KEEPER);
  UINT32 i = 0;
  M2MB_GPIO_VALUE_E initialValue;
  UINT8 val;

  if(!info)
  {
    return FALSE;
  }

  if ( -1 == m2mb_gpio_read( info->fd, &initialValue) )
  {
    AZX_LOG_ERROR("Failed to read GPIO %u\r\n", gpio);
    return FALSE;
  }
  else
  {
    AZX_LOG_TRACE("Initial value: %d\r\n", initialValue);
  }

  val = !initialValue;

  /*now open in output mode*/
  info = do_gpio_conf(gpio, M2MB_GPIO_MODE_OUTPUT, M2MB_GPIO_PULL_KEEPER);
  if(!info)
  {
    return FALSE;
  }


  AZX_LOG_TRACE("Start pulse\r\n");

  //TODO: look into HW timers
  const UINT32 sleep_in_ticks = SLEEP_IN_TICKS /*30/120/2/2*/*(period); // Quite inaccurate
  AZX_LOG_TRACE("Sleeping for: %d ticks\r\n", sleep_in_ticks);
  for(i=0; i < no_of_waves*2; ++i)
  {
    volatile UINT32 s = 0;
    // Currently this operation takes ~7.6E-6s
    if ( -1 == m2mb_gpio_write( info->fd, val ? M2MB_GPIO_HIGH_VALUE : M2MB_GPIO_LOW_VALUE) )
    {
      AZX_LOG_ERROR("Failed to write %d to GPIO %u \r\n", val, gpio);
      return FALSE;
    }
    val = !val;

    for(s=0; s < sleep_in_ticks ;++s)
    {
    }
  }
  // End with what it started with
  m2mb_gpio_write( info->fd, initialValue ? M2MB_GPIO_HIGH_VALUE : M2MB_GPIO_LOW_VALUE);

  AZX_LOG_TRACE("End pulse\r\n");

  return TRUE;
}

static void m2mb_cb(UINT32 fd, void *userdata)
{
  M2MB_GPIO_VALUE_E val;
  InterruptRequest* irq = (InterruptRequest*) userdata;

  if(irq == NULL || irq->cb == NULL)
  {
    AZX_LOG_ERROR("Received interrupt but the irq struct is invalid\r\n");
    return;
  }

  if ( -1 == m2mb_gpio_read( fd, &val) )
  {
    AZX_LOG_ERROR("Failed to read GPIO %u\r\n", irq->gpio);
    return;
  }

  if (!azx_timer_hasTimestampPassed(irq->debounce_until_ts))
  {
    AZX_LOG_TRACE("Debouncing interrupt from GPIO %u\r\n", irq->gpio);
    return;
  }
  irq->debounce_until_ts = azx_timer_getTimestampFromNow(irq->cooldown_interval_ms);

  AZX_LOG_DEBUG("Received interrupt from gpio %u val=%d\r\n", irq->gpio, val);
  irq->cb(irq->gpio, (val == M2MB_GPIO_LOW_VALUE ? AZX_GPIO_LOW : AZX_GPIO_HIGH));
}

BOOLEAN azx_gpio_registerInterrupt(UINT8 gpio, AZX_GPIO_TRIGGER_E trigger,
    M2MB_GPIO_PULL_MODE_E pull,
    azx_gpio_interrupt_cb* cb, UINT32 debounce_cooldown_interval_ms)
{
  GpioInfo* info = prepare_gpio(gpio);
  InterruptRequest* irq = get_irq(gpio);

  if(!info || !irq)
  {
    goto error;
  }

  if(irq->cb)
  {
    AZX_LOG_WARN("Replacing existing cb\r\n");
  }

  if(0 != m2mb_gpio_multi_ioctl(info->fd, CMDS_ARGS(
          M2MB_GPIO_IOCTL_SET_DIR, M2MB_GPIO_MODE_INPUT,
          M2MB_GPIO_IOCTL_SET_PULL, pull,
          M2MB_GPIO_IOCTL_SET_INTR_TRIGGER, trigger,
          M2MB_GPIO_IOCTL_SET_INTR_TYPE, INTR_CB_SET,
          M2MB_GPIO_IOCTL_SET_INTR_CB, m2mb_cb,
          M2MB_GPIO_IOCTL_SET_INTR_ARG, irq,
          M2MB_GPIO_IOCTL_INIT_INTR, NULL
        )))
  {
    AZX_LOG_WARN("M2MB ioctl request failed\r\n");
    goto error;
  }

  irq->cb = cb;
  irq->gpio = gpio;
  irq->trigger = trigger;
  irq->debounce_until_ts = 0;
  irq->cooldown_interval_ms = debounce_cooldown_interval_ms;

  AZX_LOG_DEBUG("Registered interrupt for GPIO %u at %s\r\n", gpio,
      strgpiotrig(trigger));

  return TRUE;
error:
  AZX_LOG_ERROR("Failed to register interrupt for GPIO %u\r\n", gpio);
  if(irq)
  {
    memset(irq, 0, sizeof(InterruptRequest));
  }

  return FALSE;
}
