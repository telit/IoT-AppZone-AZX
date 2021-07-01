/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef UUID_06d153c6_a8e0_4f22_bc6b_891d90a9a4d7
#define UUID_06d153c6_a8e0_4f22_bc6b_891d90a9a4d7
/**
 * @file azx_adc.h
 * @version 1.0.2
 * @dependencies core/azx_string core/azx_ati
 * @author Ioannis Demetriou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Read from and write to a peripheral via ADC
 *
 * This library makes it easy to write and read ADC values from a GPIO pin.
 * This is achieved by using AT commands.
 *
 *
 * @warning There is no mechanism to set an ADC value.
 */
#include "m2mb_types.h"
#include "azx_ati.h"
#include "azx_string.h"


/**
 * @brief Value reported in case the ADC read fails.
 */
#define AZX_ADC_READ_ERROR 0xFFFF


/**
 * @brief Gets the value of an ADC pin.
 *
 *
 * @param[in] adc The pin to read. Use 1
 *
 * @return The value read. In case of error, @ref AZX_ADC_READ_ERROR is returned.
 */
UINT16 azx_adc_get(UINT8 adc);


#endif /* !defined( UUID_06d153c6_a8e0_4f22_bc6b_891d90a9a4d7 ) */
