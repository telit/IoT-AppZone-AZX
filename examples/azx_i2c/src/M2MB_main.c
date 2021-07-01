/* Copyright (C) 2021 Telit Technologies. All Rights Reserved.*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"

#include "app_cfg.h"

#include "azx_log.h"

#include "azx_i2c.h"

#define I2C_SDA       (UINT8) 2
#define I2C_SCL       (UINT8) 3


#define KX_I2C_ADDR      (UINT16) 0x0F

#define KX_WHOAMI        (UINT8) 0x0F

#define KX_CTRL_REG1     (UINT8) 0x1B

#define KX_CTRL_REG3  (UINT8) 0x1D  //4D     01001101

#define KX_X_OUT_LSB   (UINT8) 0x06
#define KX_X_OUT_MSB   (UINT8) 0x07
#define KX_Y_OUT_LSB   (UINT8) 0x08
#define KX_Y_OUT_MSB   (UINT8) 0x09
#define KX_Z_OUT_LSB   (UINT8) 0x0A
#define KX_Z_OUT_MSB   (UINT8) 0x0B


UINT16 axisValue(UINT8 msB, UINT8 lsB)
{
  return ( msB << 4) + (lsB >> 4);
}


float twoscompl(INT16 binaryValue)
{
  UINT8 isNegative;
  INT16 temp;
  float result;

  isNegative = (binaryValue & (1 << 11)) != 0; //if MSB == 1 -> negative number

  if (isNegative)
  {
      temp = binaryValue | ~((1 << 12) - 1);
      result = (float)(~temp + 1 ) * -1999/2047000.0;  //returns values with sign
  }
  else
      result = binaryValue * 1999/2047000.0;

  return result;
}

void M2MB_main( int argc, char **argv )
{
  (void)argc;
  (void)argv;
  UINT8 data[32]  = {0};
  m2mb_os_taskSleep(M2MB_OS_MS2TICKS(4000));

  AZX_LOG_INIT();

  AZX_LOG_INFO("Azx-i2c demo. \r\n");


  AZX_LOG_INFO("Setting I2C pins..\r\n");
  azx_i2c_set_pins(I2C_SDA, I2C_SCL);

  AZX_LOG_INFO( "\r\nReading Accelerometer WhoAmI register...\r\n" );

  if(TRUE == azx_i2c_read(KX_I2C_ADDR, KX_WHOAMI, data, 1))
  {
    AZX_LOG_INFO("Read succeeded, register value is 0x%02X\r\n", data[0]);
  }
  else
  {
    AZX_LOG_ERROR("Cannot read from device!");
    return;
  }

/* CTRL_REG3 */
  AZX_LOG_INFO("Setting accelerometer to ODR tilt: 12.5Hz, ODR directional tap: 400Hz, ORD Motion Wakeup: 50Hz\r\n");
  data[0] = 0x4D;
  if(TRUE == azx_i2c_write( KX_I2C_ADDR, KX_CTRL_REG3, data, 1))
  {
    AZX_LOG_INFO("Done. checking set value...\r\n");

    memset(data, 0, sizeof(data));
    if(TRUE == azx_i2c_read(KX_I2C_ADDR, KX_CTRL_REG3, data, 1))
    {
      if (data[0] == 0x4D)
      {
        AZX_LOG_INFO("Expected value!\r\n");
      }
      else
      {
        AZX_LOG_INFO("Read succeeded, but register value is 0x%02X\r\n", data[0]);
      }
    }
    else
    {
      AZX_LOG_ERROR("Cannot read from device!");
      return;
    }

  }

  /* CTRL_REG1 */
  AZX_LOG_INFO("Setting accelerometer to Operative mode, 12bit resolution\r\n");
  data[0] = 0xC0;
  if(TRUE == azx_i2c_write( KX_I2C_ADDR, KX_CTRL_REG1, data, 1))
  {
    AZX_LOG_INFO("Done. checking set value...\r\n");

    memset(data, 0, sizeof(data));
    if(TRUE == azx_i2c_read(KX_I2C_ADDR, KX_CTRL_REG1, data, 1))
    {
      if (data[0] == 0xC0)
      {
        AZX_LOG_INFO("Expected value!\r\n");
      }
      else
      {
        AZX_LOG_INFO("Read succeeded, but register value is 0x%02X\r\n", data[0]);
      }
    }
    else
    {
      AZX_LOG_ERROR("Cannot read from device!");
      return;
    }
  }

 AZX_LOG_INFO( "I2C read axes registers\r\n" );
 AZX_LOG_INFO( "------------\r\n" );

  memset(data, 0, sizeof(data));
  if(TRUE == azx_i2c_read(KX_I2C_ADDR, KX_X_OUT_LSB, data, 6))
  {
    UINT16 xAxis12bit, yAxis12bit, zAxis12bit;
    xAxis12bit = axisValue( data[1], data[0] );  //msb, lsb
    yAxis12bit = axisValue( data[3], data[2] );
    zAxis12bit = axisValue( data[5], data[4] );
    
    AZX_LOG_INFO( "\r\nX: %.3f g\r\nY: %.3f g\r\nZ: %.3f g\r\n", twoscompl(xAxis12bit), twoscompl(yAxis12bit), twoscompl(zAxis12bit) );
  }
  else
  {
    AZX_LOG_ERROR("Cannot read from device!");
    return;
  }

}
