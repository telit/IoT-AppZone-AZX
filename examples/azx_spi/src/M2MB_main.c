/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_gpio.h"
#include "azx_spi.h"


#define SPI_BUF_LEN 8

/*
 * ST_LIS3MDL magnetometer
 * REQUEST BYTE FORMAT:
 *  |RW|MS|ADDR|
 *  where
 *  RW: when 0, data in MOSI(7:0) is written into the device. when 1, data MISO(7:0) is read from the device.
 *  In the latter case, the device will drive MISO at the start of bit 8
 *  MS: when 0, the address will remain unchanged in multiple read/write commands. When 1, it is auto-incremented
 *  ADDR (5:0) the address of the indexed register
 */

#define ST_RW_BIT 1<<7
#define ST_MS_BIT 1<<6 //multiple send

//ST Magnetometer defines
#define ST_LIS3MDL_CS_PIN 2  //gpio 2
#define ST_LIS3MDL_WHOAMI 0x0F

#define ST_LIS3MDL_OUT_X_L 0x28
#define ST_LIS3MDL_OUT_X_H 0x29
#define ST_LIS3MDL_OUT_Y_L 0x2A
#define ST_LIS3MDL_OUT_Y_H 0x2B
#define ST_LIS3MDL_OUT_Z_L 0x2C
#define ST_LIS3MDL_OUT_Z_H 0x2D

#define ST_LIS3MDL_CTRL_REG1 0x20
#define ST_LIS3MDL_CTRL_REG2 0x21
#define ST_LIS3MDL_CTRL_REG3 0x22
#define ST_LIS3MDL_CTRL_REG4 0x23
#define ST_LIS3MDL_CTRL_REG5 0x24



/*
 *  Connection scheme
  #####################################################################
                     | CS1467g-A       | ST
  TX_AUX/MOSI        | PL303/1         | SDI
  RX_AUX/MISO        | PL303/2         | SDO
  SPI_CLK            | PL303/3         | SCK
  GPIO2              | PL302/2         | CS for Magnetometer
  GPIO3              | PL302/3         | CS for Gyroscope
  GND                | PL303/10        | BLACK
  V3.8               | PL101/9         | RED
  #####################################################################
  PL101/9 actually provides a 3.9xxV voltage, above the 3.8V max (according to specifications).
  USIF1 (aux UART) will be occupied since the pins are shared with SPI.
*/


void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;
  
  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INIT();

  AZX_LOG_INFO("Azx-spi demo. \r\n");


  if(FALSE == azx_spi_open(3, 0 /*CPOL 0, CPHA 0*/, 1 /*speed*/, 8 /*bits per word*/))
  {
    AZX_LOG_ERROR("cannot open SPI channel!\r\n");
    return;
  }
  else
  {
    UINT8 tx_buf[2];
    UINT8 rx_buf[2];
    BOOLEAN ret_spi;
    memset(tx_buf, 0xFF, 2);
    memset(rx_buf, 0, 2);

    AZX_LOG_INFO("Reading magnetometer WhoAmI register...\r\n");
    tx_buf[0] = ST_LIS3MDL_WHOAMI | ST_RW_BIT; //add RW bit since this is a read request

    /*
     * ST REQUEST BYTE FORMAT:
     *  |RW|MS|ADDR|
     *  where
     *  RW: when 0, data in MOSI(7:0) is written into the device. when 1, data MISO(7:0) is read from the device.
     *  In the latter case, the device will drive MISO at the start of bit 8
     *  MS: when 0, the address will remain unchanged in multiple read/write commands. When 1, it is auto-incremented
     *  ADDR (5:0) the address of the indexed register
     */

    AZX_LOG_TRACE("Sending: %02X %02X \r\n",tx_buf[0], tx_buf[1] );

    azx_gpio_set(ST_LIS3MDL_CS_PIN, AZX_GPIO_LOW); //set CS to output LOW
    ret_spi = azx_spi_write(3, tx_buf, rx_buf, 2);
    azx_gpio_set(ST_LIS3MDL_CS_PIN, AZX_GPIO_HIGH); //set CS to output HIGH

    if(ret_spi)
    {
      AZX_LOG_TRACE("SPI RAW recv: %02X %02X\r\n", rx_buf[0], rx_buf[1]);
      AZX_LOG_INFO("Register is 0x%02X\r\n", rx_buf[1]);
      if(0x3D == rx_buf[1])
      {
        AZX_LOG_INFO("Expected value!\r\n");
      }
    }
    else
    {
      AZX_LOG_ERROR("Failed reading from SPI device\r\n");
      return;
    }

    AZX_LOG_INFO("Setting continuous conversion mode...\r\n");

    memset(tx_buf, 0, 2);
    memset(rx_buf, 0, 2);

    tx_buf[0] = ST_LIS3MDL_CTRL_REG3 & (~(ST_RW_BIT | ST_MS_BIT)); //remove RW bit since this is a write request
    tx_buf[1] = 0x00;

    azx_gpio_set(ST_LIS3MDL_CS_PIN, AZX_GPIO_LOW); //set CS to output LOW
    ret_spi = azx_spi_write(3, tx_buf, rx_buf, 2);
    azx_gpio_set(ST_LIS3MDL_CS_PIN, AZX_GPIO_HIGH); //set CS to output HIGH

    if(ret_spi)
    {
      AZX_LOG_INFO("Register written\r\n");
    }
    else
    {
      AZX_LOG_ERROR("Failed writing to SPI device\r\n");
      return;
    }

    AZX_LOG_TRACE("Reading again Magnetometer CTRL_REG3.\r\n");
    
    memset(tx_buf, 0xFF, 2);
    memset(rx_buf, 0, 2);

    tx_buf[0] = ST_LIS3MDL_CTRL_REG3 | ST_RW_BIT; //add RW bit since this is a read request

    AZX_LOG_TRACE("Sending: %02X %02X \r\n",tx_buf[0], tx_buf[1] );

    azx_gpio_set(ST_LIS3MDL_CS_PIN, AZX_GPIO_LOW); //set CS to output LOW
    ret_spi = azx_spi_write(3, tx_buf, rx_buf, 2);
    azx_gpio_set(ST_LIS3MDL_CS_PIN, AZX_GPIO_HIGH); //set CS to output HIGH

    if(ret_spi)
    {
      AZX_LOG_INFO("Register is 0x%02X\r\n", rx_buf[1]);
      if(0x00 == rx_buf[1])
      {
        AZX_LOG_INFO("Continuous conversion mode successfully set.\r\n");
      }
      else
      {
        AZX_LOG_ERROR("Failed register configuration, expected 0x00, received %02X\r\n", rx_buf[1]);
        return;
      }
    }
    else
    {
      AZX_LOG_ERROR("Failed reading from SPI device\r\n");
      return;
    }
    
  }
}
