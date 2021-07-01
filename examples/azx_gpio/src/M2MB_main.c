/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_gpio.h"

void my_gpio_interrupt_cb(UINT8 gpio, UINT8 state)
{
  AZX_LOG_INFO("Interrupt occurred on GPIO %u. State is: %u\r\n", gpio, state);
}

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;

  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INIT();

   AZX_LOG_INFO("Azx-gpio demo.\r\n");

  AZX_LOG_INFO("Sending 20 pulses with 1 ms period on GPIO 3\r\n");
  azx_gpio_sendPulse(3,1000,20);


  AZX_LOG_INFO("Showcasing interrupt. GPIO 3 and GPIO 4 require to be shorted to test the interrupt functionality\r\n");

  AZX_LOG_INFO("Register a Rising Edge Interrupt on GPIO 4\r\n");
  azx_gpio_registerInterrupt(4,
      M2MB_GPIO_INTR_POSEDGE,
      M2MB_GPIO_PULL_DOWN,
      my_gpio_interrupt_cb,
      50 /*50 ms debounce*/);

  AZX_LOG_INFO("Setting GPIO 3 to output LOW\r\n");
  azx_gpio_set(3, AZX_GPIO_LOW);

  azx_sleep_ms(500);

  AZX_LOG_INFO("Setting GPIO 3 to output HIGH\r\n");
  azx_gpio_set(3, AZX_GPIO_HIGH);

  azx_sleep_ms(3000);

  AZX_LOG_INFO("Setting GPIO 3 to output LOW\r\n");
  azx_gpio_set(3, AZX_GPIO_LOW);
}
