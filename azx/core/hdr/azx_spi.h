/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_e01abdf5_7211_47ab_b352_b54871ecb956
#define UUID_e01abdf5_7211_47ab_b352_b54871ecb956
/**
 * @file azx_spi.h
 * @version 1.0.1
 * @dependencies core/azx_log
 * @author Ioannis Demetriou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Communicate with peripherals connected via the SPI bus
 *
 * This library simplifies the interaction with sensors using SPI.
 * You can just open the SPI channel with  azx_spi_open() and then read/write
 * with azx_spi_write().
 *
 * If you are using an SPI device that doesn't run on 8 bits, then you should
 * use azx_spi_alignAndWrite() for reading and writing.
 *
 * For full flexibility use the m2mb_spi.h API directly.
 */
#include "m2mb_types.h"
#include "azx_log.h"

/**
 * @brief If enabled, the library will use the SPI defaults
 */
/* #define AZX_SPI_UPDATE_OLD_CONFIG */
#if defined(DOXYGEN) && !defined(AZX_SPI_UPDATE_OLD_CONFIG)
#define AZX_SPI_UPDATE_OLD_CONFIG
#undef AZX_SPI_UPDATE_OLD_CONFIG
#endif

/**
 * @brief If enabled, the library will check that the bits are sent as expected.
 */
/* #define AZX_SPI_USE_LOOPBACK */
#if defined(DOXYGEN) && !defined(AZX_SPI_USE_LOOPBACK)
#define AZX_SPI_USE_LOOPBACK
#undef AZX_SPI_USE_LOOPBACK
#endif

/**
 * @brief If enabled, the library will show the bytes sent/received in the log
 */
/* #define AZX_SPI_LOG_BYTES */
#if defined(DOXYGEN) && !defined(AZX_SPI_LOG_BYTES)
#define AZX_SPI_LOG_BYTES
#undef AZX_SPI_LOG_BYTES
#endif

/**
 * @brief Opens the port to use an SPI device.
 *
 * The modem will act as the SPI master.
 *
 * @param[in] usif_num Set it to be 3.
 * @param[in] mode The mode to use (same as old `m2m_spi_api`).
      mode CPOL  CPHA  Description
      0    0     0     Data are sampled on the rising edge of the clock
      1    0     1     Data are sampled on the falling edge of the clock
      2    1     0     Data are sampled on the falling edge of the clock
      3    1     1     Data are sampled on the rising edge of the clock
      
 * @param[in] speed The speed to use (same as old `m2m_spi_api`). 
      - 1: 1 MHz
      - 2: 3.25 MHz
      - 3: 6.5MHz
      - 4: 13 MHz
 * @param[in] bits_per_word The number of bits to send out for each word.
 *     The bits must be aligned correctly before sending via azx_spi_write().
 *     This can be done with azx_spi_align().
 *
 * @return `TRUE` if the open is successful, `FALSE` otherwise.
 */
BOOLEAN azx_spi_open(INT16 usif_num, INT16 mode, INT16 speed, UINT8 bits_per_word);

/**
 * @brief Aligns the bits in the correct way for sending.
 *
 * The caller should set data in the following way:
 *  - for 3-8 bits per word: Each byte is one word (`buf` is just a `UINT8` array)
 *  - for 9-16 bits per word: Each 2 bytes is one word (`buf` is a `UINT16` array
 *    cast to `UINT8*`)
 *  - for 17-31 bits per word: Each 4 bytes is one word (`buf` is a `UINT32` array
 *    cast to `UINT8*`)
 *
 * @param[in] bits_per_word Use the value that was passed to azx_spi_open().
 * @param[in,out] buf The buffer where the bits will be aligned. All bits are
 *     aligned in-place.
 * @param[in] len The size in bytes of the buffer to align. For 9-16 must be an
 *     even number, for 17-31 must be a multiple of 4.
 */
void azx_spi_align(UINT8 bits_per_word, UINT8* buf, INT16 len);

/**
 * @brief Writes some bits on the wire and if enabled, it also reads the responses.
 *
 * @warning This call will assume that azx_spi_align() has been called before to
 *     align the bits as expected.
 *
 * @param[in] usif_num Set it to be 3.
 * @param[in] bufferToSend The buffer that is being sent out.
 * @param[out] bufferReceive The buffer where the result is being read.
 * @param[in] len The number of bytes that are being sent out.
 */
BOOLEAN azx_spi_write(INT16 usif_num, const UINT8 *bufferToSend,
    UINT8 *bufferReceive, INT16 len);

/**
 * @brief Convenience method which runs azx_spi_align() followed by azx_spi_write().
 *
 * @param[in] usif_num Set it to be 3.
 * @param[in] bits_per_word Use the value that was passed to azx_spi_open().
 * @param[in] bufferToSend The buffer that is being sent out.
 * @param[out] bufferReceive The buffer where the result is being read.
 * @param[in] len The number of bytes that are being sent out.
 */
BOOLEAN azx_spi_alignAndWrite(INT16 usif_num, UINT8 bits_per_word,
    UINT8 *bufferToSend, UINT8 *bufferReceive, INT16 len);

/**
 * @brief Closes the SPI port
 */
void azx_spi_close();

#endif /* !defined (UUID_e01abdf5_7211_47ab_b352_b54871ecb956) */
