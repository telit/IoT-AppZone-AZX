/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef LIBS_HDR_APN_H_
#define LIBS_HDR_APN_H_
/**
 * @file azx_apn.h
 * @version 1.0.3
 * @dependencies core/azx_ati core/azx_string
 * @author Demetris Constantinou
 * @author Sorin Basca
 * @date 10/02/2019
 *
 * @brief Automatically setting APN based on the ICCID of the SIM
 *
 * This library allows deployment on the modem of an `iccid.dat` file containing
 * details of what APN should be selected when a certain ICCID is found. The
 * file can contain multiple options and the best match approach is taken to
 * choose a desired value. AZX_APN_CID macro is defined in Makefile.in file and 
 * selects which PDP Context must be set with the retrieved APN and parameters.
 *
 * The format of a line in `iccids.dat` is:
 *
 *     <9 ICCID digits>:<APN string>:<apn-username | empty>:<apn-password | empty>:<1 | 2 | empty>
 *
 * For example:
 *
 *     897777777:sample:username:password:2
 *     894620460:NXT17.NET:::
 *
 * Any lines beginning with # will be ignored (and can be used for documenting
 * the entries in the file).
 *
 * The matching algorithm will retrieve the ICCID (if possible) and then set
 * the APN according to the following options:
 *  - if no SIM is found, then the APN will be AZX_APN_DEFAULT
 *  - if no match in the file, then the APN will be AZX_APN_DEFAULT
 *  - if one match in the file (one of the sequence matches the beginning of
 *  the ICCID) then the APN is selected from that
 *  - if more than one match in the file, then the longest sequence match will
 *  be chosen (for example if 891234 and 89123456 is found to match
 *  8912345678..., then the entry for 89123456 will be picked)
 *
 * You can have a default entry in iccids.dat that just has 89, or *, at the
 * beginning of the file. This means that if no match is found, this default
 * entry is used instead of @ref AZX_APN_DEFAULT.
 *
 * @see azx_apn_autoSet
 * @see azx_apn_getInfo
 */
#include "m2mb_types.h"
#include "app_cfg.h"
#include "azx_log.h"
#include "azx_string.h"
#include "azx_ati.h"

/**
 * @name Macros containing the sizes to be used for the APN info structure
 *
 * @see ApnInfo
 * @{
 */
#define _MAX_CCID_SIZE 11     /**< Max size of CCID */
#define _MAX_APN_SIZE 31      /**< Max size of APN string */
#define _MAX_USERNAME_SIZE 31 /**< Max size of APN username */
#define _MAX_PASSWORD_SIZE 31 /**< Max size of APN password */

#define MAX_CCID_SIZE (_MAX_CCID_SIZE + 1) /**< Max size of CCID buffer */
#define MAX_APN_SIZE (_MAX_APN_SIZE + 1)   /**< Max size of APN buffer */
#define MAX_USERNAME_SIZE (_MAX_USERNAME_SIZE + 1) /**< Max size of APN username buffer */
#define MAX_PASSWORD_SIZE (_MAX_PASSWORD_SIZE + 1) /**< Max size of APN password buffer */

#define MAX_CCID_SIZE_STR EXPAND_AND_QUOTE(_MAX_APN_SIZE) /**< Max size of CCID as string */
#define MAX_APN_SIZE_STR EXPAND_AND_QUOTE(_MAX_APN_SIZE)  /**< Max size of APN as string */
#define MAX_USERNAME_SIZE_STR EXPAND_AND_QUOTE(_MAX_USERNAME_SIZE) /**< Max size of APN username as string*/
#define MAX_PASSWORD_SIZE_STR EXPAND_AND_QUOTE(_MAX_PASSWORD_SIZE) /**< Max size of APN password as string*/
/** @} */

/**
 * @brief Specifies how the APN was selected.
 *
 * Once the APN is selected, this enum helps in distinguish on the selection
 * method used. This is especially useful if the selected APN is not as
 * expected and the problem needs to be investigated.
 *
 * @see ApnInfo
 */
typedef enum
{
  /**
   * @brief The default APN was used
   *
   * This means that either no SIM was present, or the `iccids.dat` file is
   * missing.
   */
  AZX_APN_SEL_DEFAULT,
  /**
   * @brief The default APN from iccids.dat was used
   *
   * This can happen if there is a default entry at the top of the file
   * containing "89" or "*"
   */
  AZX_APN_SEL_DEFAULT_FROM_FILE,
  /**
   * @brief An APN value was found in iccids.dat
   *
   * There is some match that has more than 2 digits in common with the start of
   * the ICCID.
   */
  AZX_APN_SEL_FROM_FILE,
} AZX_APN_SELECTION_E;

/**
 * @brief Structure holding the APN information
 *
 * Once the APN is selected, this structure will have the information regarding
 * the selection method and values chosen.
 *
 * @see azx_apn_autoSet
 * @see azx_apn_getInfo
 */
struct ApnInfo
{
  /**
   * @brief Selection method for the chosen APN.
   *
   * @see AZX_APN_SELECTION_E
   */
  AZX_APN_SELECTION_E selection_method;
  /**
   * @brief ICCID value for the best match.
   *
   * This will match the beginning of the ICCID of the SIM.
   */
  char ccid[MAX_CCID_SIZE];
  /**
   * @brief APN value chosen.
   */
  char apn[MAX_APN_SIZE];
  /**
   * @brief Username for logging into the APN.
   */
  char username[MAX_USERNAME_SIZE];
  /**
   * @brief Password for logging into the APN.
   */
  char password[MAX_PASSWORD_SIZE];
  /**
   * @brief The authentication type needed by the APN.
   *
   * Can be 0 (no authentication needed), 1, or 2.
   */
  INT32 auth_type;
};

/**
 * @brief Initializes the component loading the configuration.
 *
 * If azx_apn_autoSet is not used, you can just read the file configuration by calling this
 * function. The APN will not be updated, neither with the modem, nor in the memory.
 *
 * @see azx_apn_autoSet
 * @see azx_apn_recheckSim
 * @see azx_apn_getInfo
 */
void azx_apn_init(void);

/**
 * @brief Checks the SIM status again and may update the APN.
 *
 * This function checks the SIM presence and the ICCID value and will select the optimal APN for
 * that value.
 *
 * The function just stores the APN in memory and the new value can be retrieved through
 * azx_apn_getInfo. The APN does not get registered with the modem. This is useful if the APN is
 * registered though a call in another library.
 *
 * @param set_apn If the flag is true, the APN will also be set on the modem, otherwise the values
 * are updated without informing the modem of the new APN
 *
 * @see azx_apn_autoSet
 * @see azx_apn_getInfo
 */
void azx_apn_recheckSim(BOOLEAN set_apn);

/**
 * @brief Gets the details of the chosen APN values.
 *
 * Once azx_apn_autoSet() returns, this can be used to retrieve the details of
 * what was chosen.
 *
 * @return Pointer to the structure containing the information.
 *
 * @see azx_apn_autoSet
 * @see ApnInfo
 */
const struct ApnInfo* azx_apn_getInfo(void);

/**
 * @brief Automatically sets the APN.
 *
 * This will retrieve the ICCID of the SIM and decide on what APN to use based
 * on the matching algorithm and the `iccids.dat` file.
 *
 * It is advised to ensure that the SIM is plugged in before calling this and
 * also to call it again in case of SIM change. Otherwise the default APN will
 * be used, which may not be what is required.
 *
 * Once the APN is selected, this will automatically set it by running the
 * needed AT commands.
 *
 * This function is the same as calling the other functions in sequence and then calling the
 * AT+CGDCONT command to set the APN.
 *
 * @see azx_apn_getInfo
 * @see azx_apn_recheckSim
 * @see azx_apn_init
 */
void azx_apn_autoSet(void);

#endif /* LIBS_HDR_APN_H_ */
