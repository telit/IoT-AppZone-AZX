/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_i2c.h"

#include "azx_log.h"
#include "azx_i2c.h"

#define DEFAULT_I2C_SDA       (UINT8) 2  //GPIO 2
#define DEFAULT_I2C_SCL       (UINT8) 3  //GPIO 3

#define INVALID_I2C_FD -1

#define MAX_FDS 20

#if !(AZ_MODULE_FAMILY == 2520 )
#define AZX_I2C_RAW_IO
#endif

typedef struct {
  BOOLEAN is_used;
  UINT16 device_id;
  UINT8 register_id;
  INT32 fd;
} I2C_DEVICE;

static I2C_DEVICE openFds[MAX_FDS] = { 0 };
static UINT8 sdaPin = DEFAULT_I2C_SDA;
static UINT8 sclPin = DEFAULT_I2C_SCL;

static BOOLEAN configure_i2c(I2C_DEVICE* dev, UINT8 register_id)
{
  M2MB_I2C_CFG_T config = {
    .sdaPin = sdaPin,
    .sclPin = sclPin,
    .registerId = register_id,
  };
  BOOLEAN result = FALSE;

  if(register_id == dev->register_id)
  {
    return TRUE;
  }

  AZX_LOG_TRACE("Configuring device 0x%04X to use register 0x%02X\r\n", dev->device_id, register_id);
  result = (0 == m2mb_i2c_ioctl(dev->fd, M2MB_I2C_IOCTL_SET_CFG, (void *)&config) ?  TRUE : FALSE);
  if(result)
  {
    AZX_LOG_TRACE("Configuration done\r\n");
    dev->register_id = register_id;
  }
  return result;
}

static I2C_DEVICE* find_or_open_i2c(UINT16 device_id)
{
  UINT16 i = 0;

  AZX_LOG_TRACE("Finding or opening device 0x%04X\r\n", device_id);
  for(i = 0; i < MAX_FDS; ++i)
  {
    if(openFds[i].is_used && openFds[i].device_id == device_id)
    {
      AZX_LOG_TRACE("Device 0x%04X is already open and used (fd: %d)\r\n", device_id, openFds[i].fd);
      return &openFds[i];
    }
  }
  for(i = 0; i < MAX_FDS; ++i)
  {
    if(!openFds[i].is_used)
    {
      CHAR path[16];
      INT32 fd = INVALID_I2C_FD;
      /* The RW bit is set by the API*/
#ifndef AZX_I2C_NO_ADDR_SHIFT
      snprintf(path, sizeof(path), "/dev/I2C-%d", device_id << 1);
#else
      snprintf(path, sizeof(path), "/dev/I2C-%d", device_id);
#endif
      AZX_LOG_TRACE("Opening device %s\r\n", path);
      fd = m2mb_i2c_open((const CHAR*)path, 0);
      if(fd == INVALID_I2C_FD)
      {
        AZX_LOG_ERROR("Failed to open device 0x%04X\r\n", device_id);
        return NULL;
      }
      openFds[i].is_used = TRUE;
      openFds[i].device_id = device_id;
      openFds[i].fd = fd;

      AZX_LOG_TRACE("Opened device 0x%04X as fd %d\r\n", device_id, fd);
      return &openFds[i];
    }
  }
  AZX_LOG_ERROR("Not enough space to store a fd for device 0x%04X\r\n", device_id);
  return NULL;
}

static void close_fd(I2C_DEVICE* dev)
{
  if(dev)
  {
    AZX_LOG_TRACE("Closing 0x%04X\r\n", dev->device_id);
    m2mb_i2c_close(dev->fd);
    dev->is_used = FALSE;
  }
}

static INT32 open_i2c(UINT16 device_id, UINT8 register_id)
{
  I2C_DEVICE* dev = find_or_open_i2c(device_id);
  if(!dev)
  {
    AZX_LOG_ERROR("Cannot open device 0x%04X\r\n", device_id);
    goto error;
  }

  if(!configure_i2c(dev, register_id))
  {
    AZX_LOG_ERROR("Cannot configure device 0x%04X, register 0x%02X\r\n", device_id, register_id);
    goto error;
  }

  return dev->fd;
error:
  close_fd(dev);
  return INVALID_I2C_FD;
}

void azx_i2c_set_pins(UINT8 sda, UINT8 scl)
{
  AZX_LOG_INFO("I2C: Using SDA pin %u and SCL pin %u\r\n", sda, scl);
  sdaPin = sda;
  sclPin = scl;
}

BOOLEAN azx_i2c_read(UINT16 device_id, UINT8 register_id,
    UINT8* buffer, UINT32 size)
{
  INT32 fd = INVALID_I2C_FD;
  BOOLEAN result = FALSE;
  if(INVALID_I2C_FD == (fd = open_i2c(device_id, register_id)))
  {
    return FALSE;
  }

  memset(buffer, 0, size);
  result = ( -1 != m2mb_i2c_read(fd, (void*)buffer, size) ? TRUE : FALSE );

  if(!result)
  {
    AZX_LOG_ERROR("Failed to read from I2C device 0x%04X, register 0x%02X\r\n", device_id, register_id);
  }

  return result;
}

BOOLEAN azx_i2c_write(UINT16 device_id, UINT8 register_id,
    const UINT8* buffer, UINT32 size)
{
  INT32 fd = INVALID_I2C_FD;
  BOOLEAN result = FALSE;
  if(INVALID_I2C_FD == (fd = open_i2c(device_id, register_id)))
  {
    return FALSE;
  }

  result = ( -1 != m2mb_i2c_write(fd, (const void*)buffer, size) ? TRUE : FALSE );
  if(!result)
  {
    AZX_LOG_ERROR("Failed to write to I2C device 0x%04X, register 0x%02X\r\n", device_id, register_id);
  }


  return result;
}

BOOLEAN azx_i2c_raw_read_write(UINT16 device_id,
    const UINT8* out, UINT32 out_size,
    UINT8* in, UINT32 in_size)
{
#ifdef AZX_I2C_RAW_IO
  INT32 fd = INVALID_I2C_FD;
  M2MB_I2C_MSG msgs[] = {
    { .flags = I2C_M_WR, .len = (UINT16)out_size, .buf = (UINT8*)out },
    { .flags = I2C_M_RD, .len = (UINT16)in_size, .buf = in }
  };
  M2MB_I2C_RDWR_IOCTL_DATA rdwr_data = { .msgs = msgs, .nmsgs = 2 };
  M2MB_I2C_CFG_T cfg = { .sdaPin = sdaPin, .sclPin = sclPin,
    .registerId = 0, .rw_param = &rdwr_data };

  if(INVALID_I2C_FD == (fd = open_i2c(device_id, 0xFF)))
  {
    return FALSE;
  }

  if(-1 == m2mb_i2c_ioctl(fd, M2MB_I2C_IOCTL_RDWR, &cfg))
  {
    AZX_LOG_ERROR("Failed to perform raw I/O with I2C device 0x%04X\r\n", device_id);
    return FALSE;
  }

  return TRUE;
#else
  AZX_LOG_ERROR("Cannot perform raw I/O of I2C device, not supported\r\n");
  (void)device_id;
  (void)out;
  (void)out_size;
  (void)in;
  (void)in_size;
  return FALSE;
#endif
}

