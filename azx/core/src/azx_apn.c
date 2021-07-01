/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <string.h>
#include <stdio.h>
#include "m2mb_types.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_fs_posix.h"

#include "azx_log.h"
#include "app_cfg.h"
#include "azx_string.h"
#include "azx_ati.h"
#include "azx_apn.h"

#define CCIDS_FILENAME "/mod/iccids.dat"
#define COLON 58
#define LINE_LENGTH 100
#define MAX_APN_LIST_SIZE 100

static CHAR currentCcid[40] = {0};

static UINT16 myApnInfo = 0;
/* Have a Default APN entry at the beginning */
static struct ApnInfo allApnInfo[MAX_APN_LIST_SIZE] =
  {
    {
      AZX_APN_SEL_DEFAULT, "*",
      AZX_APN_DEFAULT, "", "", 0
    }
  };

/**
 * Checks whether or not a SIM card is found in the module.
 *
 */
static BOOLEAN is_sim_inserted(void)
{
  const CHAR* response = azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT, "AT+CPIN?");
  const CHAR* ready = "READY";
  CHAR *pResult;
  pResult = strstr(response, ready);

  if (!pResult)
  {
    return FALSE;
  }

  return TRUE;
}


/**
 * Get SIM's ICCID
 *
 * Queries the modem for its ICCID, and if successful it
 * parses and stores it in the static variable 'currentCcid'
 */
static BOOLEAN get_ccid(void)
{
  static const CHAR* ccid_fmt = "%*[^+]+CCID: %40[^ \r\n]";
  const CHAR* response = NULL;
  if (!is_sim_inserted())
  {
    AZX_LOG_WARN("Unable to retrieve ICCIDs, SIM is not inserted\r\n");
    currentCcid[0] = '\0';
    return FALSE;
  }

  response = azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT, "AT+CCID");
  if(1 > azx_parseStringf(response, ccid_fmt, currentCcid))
  {
    AZX_LOG_ERROR("Unable to parse ICCID\r\n");
    currentCcid[0] = '\0';
    return FALSE;
  }
  AZX_LOG_DEBUG("Found ICCID %s\r\n", currentCcid);
  return TRUE;
}


/**
 * Parse APN config file
 *
 * Reads and processes all the data from the iccids.dat file.
 * The ApnIfno struct is populated with either a matching line
 * or the defaults
 */
static BOOLEAN read_apn_config_file(void)
{
  static const CHAR* line_fmt =
    "%" EXPAND_AND_QUOTE(_MAX_APN_SIZE) "[^:]:"
    "%" EXPAND_AND_QUOTE(_MAX_APN_SIZE) "[^:]:"
    "%" EXPAND_AND_QUOTE(_MAX_USERNAME_SIZE) "[^:]:"
    "%" EXPAND_AND_QUOTE(_MAX_PASSWORD_SIZE) "[^:]:"
    "%d";

  char line[LINE_LENGTH];
  M2MB_FILE_T *ccids = NULL;
  UINT16 i = 1;
  INT32 extracted_count = 0;

  AZX_LOG_TRACE("Reading %s\r\n", CCIDS_FILENAME);
  ccids = m2mb_fs_fopen(CCIDS_FILENAME, "r");

  if (ccids == NULL)
  {
    AZX_LOG_WARN("File with ICCIDs did not open. Using default APN\r\n");
    goto err;
  }

  while (i < MAX_APN_LIST_SIZE && m2mb_fs_fgets(line, LINE_LENGTH, ccids) != NULL)
  {
    struct ApnInfo* inf = &allApnInfo[i];
    AZX_LOG_TRACE("Checking ICCID line: %s\r\n", line);
    if (line[0] == '#')
    {
      continue;
    }
    memset(inf, 0, sizeof(struct ApnInfo));
    extracted_count = azx_parseStringf(line, line_fmt, inf->ccid, inf->apn,
                                      inf->username, inf->password, &inf->auth_type);
    AZX_LOG_TRACE("Extracted count = %d\r\n", extracted_count);
    if(extracted_count < 2 || inf->ccid[0] == '\0')
    {
      /* First 2 fields mandatory and some part of the CCID must be specified */
      memset(inf, 0, sizeof(struct ApnInfo));
      continue;
    }
    if(extracted_count < 5)
    {
      inf->auth_type = 0;
    }
    if(strlen(inf->ccid) > 2)
    {
      inf->selection_method = AZX_APN_SEL_FROM_FILE;
    }
    else
    {
      inf->selection_method = AZX_APN_SEL_DEFAULT_FROM_FILE;
    }

    AZX_LOG_DEBUG("Read APN entry %d: CCID=%s, APN=%s (user=%s, pass=%s, auth_type=%d)\r\n",
        i, inf->ccid, inf->apn, inf->username, inf->password, inf->auth_type);
    ++i;
  }

  AZX_LOG_TRACE("Finished reading file\r\n");
  m2mb_fs_fclose(ccids);
  return TRUE;

err:
  AZX_LOG_DEBUG("Error reading ICCIDs file\r\n");
  if(ccids)
  {
    m2mb_fs_fclose(ccids);
  }
  return FALSE;
}

__attribute__((weak)) void apn_set(const struct ApnInfo *inf)
{
  if (!inf || inf->apn[0] == '\0')
  {
    return;
  }

  azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT, "AT+CGDCONT=%d,\"IP\",\"%s\"",
      AZX_PDP_CID, inf->apn);

  if (inf->auth_type == 0)
  {
    azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT, "AT+CGAUTH=%d,%d", AZX_PDP_CID, 0);
  }
  else
  {
    azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT, "AT+CGAUTH=%d,%d,%s,%s",
        AZX_PDP_CID, inf->auth_type, inf->username, inf->password);
  }
}

static struct ApnInfo* select_apn_from_ccid(void)
{
  UINT16 best_match = 0;
  UINT16 best_match_len = 0;
  UINT16 i = 1;
  struct ApnInfo* inf = &allApnInfo[best_match];

  AZX_LOG_TRACE("Selecting APN from CCID\r\n");

  if(currentCcid[0] != '\0')
  {
    while(i < MAX_APN_LIST_SIZE && allApnInfo[i].ccid[0] != '\0')
    {
      UINT16 len = strlen(allApnInfo[i].ccid);
      AZX_LOG_TRACE("Checking %d\r\n", i);
      if(len > best_match_len && strncmp(currentCcid, allApnInfo[i].ccid, len) == 0)
      {
        AZX_LOG_TRACE("Found better CCID match: %s\r\n", allApnInfo[i].ccid);
        best_match = i;
        best_match_len = len;
      }
      ++i;
    }
  }
  else
  {
    AZX_LOG_DEBUG("No ICCIDs info, using default APN\r\n");
  }

  inf = &allApnInfo[best_match];
  myApnInfo = best_match;
  AZX_LOG_INFO("APN to use: %s (CCID prefix %s)\r\n", inf->apn, inf->ccid);
  return inf;
}

void azx_apn_init(void)
{
  azx_ati_sendCommand(AZX_ATI_DEFAULT_TIMEOUT, "AT+CGDCONT=%d,\"IP\"", AZX_PDP_CID);
  read_apn_config_file();
}

void azx_apn_recheckSim(BOOLEAN set_apn)
{
  if(get_ccid())
  {
    struct ApnInfo* inf = select_apn_from_ccid();
    if(set_apn)
    {
      apn_set(inf);
    }
  }
}

/**
 * Choose an APN based on the SIM's ICCID and the config file
 *
 * This is the main function to run when we want to auto-set the APN.
 * It will run all appropriate functions in the order that makes sense
 * in order to find and set the APN based on ICCID.
 *
 * Steps: 1. It reads the ICCID of the modem SIM
 *        2. Tries to match it's first 9 characters in the file 'iccids.dat'
 *        3. Sets the APN assigned to struct myApnInfo as the modems APN.
 *        4. If the above step is unsuccessful, it sets a default APN.
 */
void azx_apn_autoSet(void)
{
  struct ApnInfo* inf = &allApnInfo[0];
  if(get_ccid() && read_apn_config_file())
  {
    inf = select_apn_from_ccid();
  }

  apn_set(inf);
}

const struct ApnInfo* azx_apn_getInfo(void)
{
  return &allApnInfo[myApnInfo];
}
