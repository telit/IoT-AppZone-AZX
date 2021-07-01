/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <stdio.h>
#include <string.h>
#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_spi.h"

#include "azx_log.h"

#include "azx_spi.h"

#define INVALID_SPI_FD -1
static INT32 fd = INVALID_SPI_FD;

static UINT32 convert_speed_to_hz(INT16 speed)
{
  switch(speed)
  {
    case 1:
      return 1000 * 1000;
    case 2:
      return 3250 * 1000;
    case 3:
      return 6500 * 1000;
    case 4:
      return 13000 * 1000;
    default:
      break;
  }
  return 1000 * 1000; /* By default use 1MHz */
}

static BOOLEAN configure_spi(INT16 mode, INT16 speed, UINT8 bits_per_word)
{
  M2MB_SPI_CFG_T cfg;
  memset(&cfg, 0, sizeof(cfg));
#ifdef AZX_SPI_UPDATE_OLD_CONFIG
  if(-1 == m2mb_spi_ioctl(fd, M2MB_SPI_IOCTL_GET_CFG, (void*)&cfg))
  {
    return FALSE;
  }
#else
  cfg.callback_ctxt = NULL;
  cfg.callback_fn = NULL;
  cfg.cs_clk_delay_cycles = 8;
  cfg.cs_mode = M2MB_SPI_CS_DEASSERT;
  cfg.cs_polarity = M2MB_SPI_CS_ACTIVE_LOW;
  cfg.inter_word_delay_cycles = 8;
#endif

#ifdef AZX_SPI_USE_LOOPBACK
  cfg.loopback_mode = TRUE;
#else
  cfg.loopback_mode = FALSE;
#endif

  cfg.spi_mode = (M2MB_SPI_SHIFT_MODE_T)mode; /* The new SPI mode enum has the same values as the old API */
  cfg.clk_freq_Hz = convert_speed_to_hz(speed);
  cfg.endianness = M2MB_SPI_BIG_ENDIAN;
  cfg.bits_per_word = bits_per_word;

  if(-1 == m2mb_spi_ioctl(fd, M2MB_SPI_IOCTL_SET_CFG, (void*)&cfg))
  {
    AZX_LOG_ERROR("Unable to configure SPI\r\n");
    return FALSE;
  }

  AZX_LOG_DEBUG("SPI configured successfully\r\n");
  return TRUE;
}

BOOLEAN azx_spi_open(INT16 usif_num, INT16 mode, INT16 speed, UINT8 bits_per_word)
{
  (void)usif_num;
  if(INVALID_SPI_FD != fd)
  {
    goto end;
  }
  if(INVALID_SPI_FD == (fd = m2mb_spi_open("/dev/spidev5.0", 0)))
  {
    AZX_LOG_ERROR("Unable to open SPI\r\n");
    goto end;
  }
  AZX_LOG_DEBUG("SPI channel opened successfully\r\n");

  if(!configure_spi(mode, speed, bits_per_word))
  {
    m2mb_spi_close(fd);
    fd = INVALID_SPI_FD;
  }

end:
  return (INVALID_SPI_FD != fd);
}

void azx_spi_align(UINT8 bits_per_word, UINT8* buf, INT16 len)
{
  if(bits_per_word < 9)
  {
    /* Everything should be aligned as needed for 3-8 bits */
    return;
  }
  if(bits_per_word < 17)
  {
    UINT16* p = (UINT16*)buf;
    const UINT8 shift = bits_per_word-8;
    if(len % 2 != 0)
    {
      /* Something is quite wrong, don't do anything */
      AZX_LOG_ERROR("Buffer size %d is not right for %u bits per word\r\n", len, bits_per_word);
      return;
    }
    AZX_LOG_DEBUG("Aligning %u bits\r\n", bits_per_word);
    while(len > 0)
    {
      UINT8 bit0 = (((*p) >> shift) & 0xFF);
      UINT8 bit1 = ((*p) & ~(0xFF << shift));

#ifdef AZX_SPI_LOG_BYTES
      AZX_LOG_DEBUG("%02X %02X -> %02X %02X\r\n", ((*p >> 8) & 0xFF), (*p & 0xFF), bit0, bit1);
#endif
      *buf++ = bit0;
      *buf++ = bit1;
      ++p;
      len -= 2;
    }
    return;

  }
  if(bits_per_word < 32)
  {
    UINT32* p = (UINT32*)buf;
    const UINT8 shift = bits_per_word-8;
    if(len % 4 != 0)
    {
      /* Something is quite wrong, don't do anything */
      AZX_LOG_ERROR("Buffer size %d is not right for %u bits per word\r\n", len, bits_per_word);
      return;
    }
    AZX_LOG_DEBUG("Aligning %u bits\r\n", bits_per_word);
    while(len > 0)
    {
      UINT8 bit0 = (((*p) >> (shift+16)) & 0xFF);
      UINT8 bit1 = (((*p) >> (shift+8)) & 0xFF);
      UINT8 bit2 = (((*p) >> shift) & 0xFF);
      UINT8 bit3 = ((*p) & ~(0xFF << shift));

#ifdef AZX_SPI_LOG_BYTES
      AZX_LOG_DEBUG("%02X %02X %02X %02X -> %02X %02X %02X %02X\r\n",
          ((*p >> 24) & 0xFF), ((*p >> 16) & 0xFF),
          ((*p >> 8) & 0xFF), (*p & 0xFF),
          bit0, bit1, bit2, bit3);
#endif
      *buf++ = bit0;
      *buf++ = bit1;
      *buf++ = bit2;
      *buf++ = bit3;
      ++p;
      len -= 4;
    }
    return;
  }
  /* We shouldn't really get here */
  AZX_LOG_ERROR("Unsupported bits per word value: %u\r\n", bits_per_word);
}

BOOLEAN azx_spi_write(INT16 usif_num, const UINT8 *bufferToSend,
    UINT8 *bufferReceive, INT16 len)
{
  (void)usif_num;
  if(fd == INVALID_SPI_FD)
  {
    return FALSE;
  }

#ifdef AZX_SPI_LOG_BYTES
  AZX_LOG_DEBUG("Sending:");
  for(int i = 0; i < len; ++i)
  {
    AZX_LOG_DEBUG(" %02X", bufferToSend[i]);
  }
  AZX_LOG_DEBUG("\r\n");

  if(bufferReceive)
  {
    AZX_LOG_DEBUG("Receive buffer:");
    for(int i = 0; i < len; ++i)
    {
      AZX_LOG_DEBUG(" %02X", bufferReceive[i]);
    }
    AZX_LOG_DEBUG("\r\n");
  }
#endif

  if(!bufferReceive)
  {
    if(len != m2mb_spi_write(fd, (void*)bufferToSend, (SIZE_T)len))
    {
      AZX_LOG_WARN("SPI didn't send the full amount of bytes\r\n");
      return FALSE;
    }
    return TRUE;
  }

  if(len != m2mb_spi_write_read(fd, (void*)bufferToSend,
        (void*)bufferReceive, (SIZE_T)len))
  {
    AZX_LOG_WARN("SPI didn't send the full amount of bytes\r\n");
    return FALSE;
  }

#ifdef AZX_SPI_LOG_BYTES
  AZX_LOG_DEBUG("Received:");
  for(int i = 0; i < len; ++i)
  {
    AZX_LOG_DEBUG(" %02X", bufferReceive[i]);
  }
  AZX_LOG_DEBUG("\r\n");
#endif
  return TRUE;
}

BOOLEAN azx_spi_alignAndWrite(INT16 usif_num, UINT8 bits_per_word,
    UINT8 *bufferToSend,
    UINT8 *bufferReceive, INT16 len)
{
  azx_spi_align(bits_per_word, bufferToSend, len);
  return azx_spi_write(usif_num, bufferToSend, bufferReceive, len);
}

void azx_spi_close()
{
  if(fd != INVALID_SPI_FD)
  {
    m2mb_spi_close(fd);
    fd = INVALID_SPI_FD;
  }
}
