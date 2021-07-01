/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_api.h"
#include "m2mb_net.h"
#include "m2mb_socket.h"
#include "m2mb_pdp.h"

#include "app_cfg.h"
#include "azx_log.h"
#include "azx_utils.h"
#include "azx_timer.h"
#include "azx_apn.h"

#include "azx_connectivity.h"

#define RETRY_MS 200
#define INFO_GET_WAIT_MS 1000

/*
 * Local Global Variables
 */
static AZX_CONNECTIVITY_INFO_T connectivityInfo = { 
  /* .status */ M2MB_NET_STAT_UNKNOWN,
  /* .rat */    M2MB_NET_RAT_UNKNOWN,
  /* .rssi */   99,
  /* .ber  */   99
};

static M2MB_NET_HANDLE netHandle = NULL;
static M2MB_OS_SEM_HANDLE regLockHandle = M2MB_OS_SEM_INVALID;
static M2MB_PDP_HANDLE pdpHandle;
static M2MB_OS_SEM_HANDLE pdpLockHandle = M2MB_OS_SEM_INVALID;

static volatile UINT32 netEvents = 0;
static volatile UINT32 expectedNetEvents = 0;

#define PDP_EVENT_UP 1
#define PDP_EVENT_DOWN 2
static volatile BOOLEAN waitingPdpEvent = 0;
static volatile INT32 pdpEvent = 0;

#define CONNECTED_EVENT_RESP 30

M2MB_NET_RAT_E azx_connectivity_getRat()
{
  return azx_connectivity_getInfo()->rat;
}

INT8 azx_connectivity_getRssi()
{
  return azx_connectivity_getInfo()->rssi;
}

static UINT32 event_bit_for_id(UINT32 event_id)
{
  return (1u << event_id);
}

static void wait_for_net_event(UINT32 event, BOOLEAN is_ind)
{
  const UINT32 bit = event_bit_for_id(event);
  expectedNetEvents |= bit;
  netEvents &= (~bit);
  if(is_ind)
  {
    m2mb_net_enable_ind(netHandle, (M2MB_NET_IND_E)event, 1);
  }
}

static void stop_waiting_for_net_event(UINT32 event, BOOLEAN is_ind)
{
  const UINT32 bit = event_bit_for_id(event);
  expectedNetEvents &= (~bit);
  netEvents &= (~bit);
  if(is_ind)
  {
    m2mb_net_enable_ind(netHandle, (M2MB_NET_IND_E)event, 0);
  }
}

static void net_cb(M2MB_NET_HANDLE h, M2MB_NET_IND_E net_event, UINT16 resp_size, void *resp_struct, void *myUserdata)
{
  (void)h;
  (void)resp_size;
  (void)myUserdata;
  switch (net_event)
  {
    case M2MB_NET_REG_STATUS_IND:
      /* Falling through as the response objects of */
      /* M2MB_NET_REG_STATUS_IND and M2MB_NET_GET_REG_STATUS_INFO_RESP are the same */
    case M2MB_NET_GET_REG_STATUS_INFO_RESP:
    {
      M2MB_NET_REG_STATUS_T *stat_info = (M2MB_NET_REG_STATUS_T *)resp_struct;
      AZX_LOG_TRACE("Network: status: %d, RAT: %d, AREA CODE: 0x%X, CELL ID: 0x%X\r\n",
          stat_info->stat, stat_info->rat, stat_info->areaCode, stat_info->cellID);
      if (stat_info->stat == M2MB_NET_STAT_REGISTERED_HOME ||
          stat_info->stat == M2MB_NET_STAT_REGISTERED_ROAMING)
      {
        AZX_LOG_DEBUG("Modem is registered to cell 0x%X\r\n", stat_info->cellID);
        if(expectedNetEvents)
        {
          netEvents |= event_bit_for_id(CONNECTED_EVENT_RESP);
        }
      }
      connectivityInfo.status = stat_info->stat;
      connectivityInfo.rat = stat_info->rat;
      break;
    }
    case M2MB_NET_GET_SIGNAL_INFO_RESP:
    {
      M2MB_NET_GET_SIGNAL_INFO_RESP_T *resp = (M2MB_NET_GET_SIGNAL_INFO_RESP_T *)resp_struct;
      connectivityInfo.rssi = resp->rssi;
      break;
    }

    case M2MB_NET_GET_BER_RESP:
    {
      M2MB_NET_GET_BER_RESP_T *resp = (M2MB_NET_GET_BER_RESP_T *)resp_struct;
      connectivityInfo.ber = resp->ber;
      break;
    }

    default:
      AZX_LOG_DEBUG("unexpected net_event: %d\r\n", net_event);
      break;
  }

  if(expectedNetEvents)
  {
    netEvents |= event_bit_for_id(net_event);

    if(net_event == M2MB_NET_GET_REG_STATUS_INFO_RESP)
    {
      /*If URC event was requested and status info response was received, just disable the URC flag*/
      if( (event_bit_for_id(M2MB_NET_REG_STATUS_IND) & expectedNetEvents) ==
          event_bit_for_id(M2MB_NET_REG_STATUS_IND))
      {
        /* Stop network status updates */
        stop_waiting_for_net_event(M2MB_NET_REG_STATUS_IND, TRUE);
      }
    }

    if((netEvents & expectedNetEvents) == expectedNetEvents)
    {
      m2mb_os_sem_put(regLockHandle);
    }
  }
}

static BOOLEAN init_net_resources()
{
  if (regLockHandle == M2MB_OS_SEM_INVALID)
  {
    M2MB_OS_SEM_ATTR_HANDLE semaphore_attrs;
    m2mb_os_sem_setAttrItem(&semaphore_attrs,
        CMDS_ARGS(
          M2MB_OS_SEM_SEL_CMD_CREATE_ATTR, NULL,
          M2MB_OS_SEM_SEL_CMD_COUNT, 0 /*IPC*/,
          M2MB_OS_SEM_SEL_CMD_TYPE, M2MB_OS_SEM_BINARY,
          M2MB_OS_SEM_SEL_CMD_NAME, "regSem"));
    m2mb_os_sem_init( &regLockHandle, &semaphore_attrs );
  }

  if(netHandle == NULL)
  {
    if(M2MB_RESULT_SUCCESS != m2mb_net_init(&netHandle, net_cb, NULL))
    {
      AZX_LOG_ERROR("Unable to init m2mb_net API\r\n");
      return FALSE;
    }
  }

  return TRUE;
}

static BOOLEAN wait_for_network_connection(UINT32 timeout_ms)
{
  /* Enable network status updates to avoid spamming the firmware */
  wait_for_net_event(M2MB_NET_REG_STATUS_IND, TRUE);
  wait_for_net_event(CONNECTED_EVENT_RESP, FALSE);

  /* Get the current network status, in case the modem is already registered */
  if(M2MB_RESULT_SUCCESS != m2mb_net_get_reg_status_info(netHandle))
  {
    AZX_LOG_ERROR("Unable to request network status update");
    return FALSE;
  }

  M2MB_OS_RESULT_E rc = m2mb_os_sem_get(regLockHandle,
      (timeout_ms == TRY_FOREVER ? M2MB_OS_WAIT_FOREVER : M2MB_OS_MS2TICKS(timeout_ms)));

  /* Stop network status updates */
  stop_waiting_for_net_event(M2MB_NET_REG_STATUS_IND, TRUE);
  wait_for_net_event(CONNECTED_EVENT_RESP, FALSE);

  if(rc != M2MB_OS_SUCCESS)
  {
    AZX_LOG_WARN("Sync connect block has timed-out (err: %d)\r\n", rc);
    return FALSE;
  }

  return TRUE;
}

BOOLEAN azx_connectivity_ConnectToNetworkSyncronously(UINT32 timeout_sec)
{
  UINT32 timeout_ms = timeout_sec * 1000;
  UINT32 wait_until = azx_timer_getTimestampFromNow(timeout_ms);

  AZX_LOG_DEBUG("Waiting for network registration\r\n");

  while(!azx_timer_hasTimestampPassed(wait_until))
  {
    init_net_resources();
    if(wait_for_network_connection(timeout_ms))
    {
      return TRUE;
    }

    azx_sleep_ms(RETRY_MS);
  }

  return FALSE;
}

static void pdp_cb(M2MB_PDP_HANDLE h, M2MB_PDP_IND_E pdp_event, UINT8 cid, void *userdata)
{
  struct M2MB_SOCKET_BSD_SOCKADDR_IN CBtmpAddress;
  CHAR CBtmpIPaddr[32];
  (void)userdata;

  switch (pdp_event)
  {
    case M2MB_PDP_UP:
      AZX_LOG_DEBUG("PDP context activated\r\n");
      m2mb_pdp_get_my_ip(h, cid, M2MB_PDP_IPV4, &CBtmpAddress.sin_addr.s_addr);
      m2mb_socket_bsd_inet_ntop( M2MB_SOCKET_BSD_AF_INET, &CBtmpAddress.sin_addr.s_addr, ( CHAR * )&( CBtmpIPaddr ), sizeof( CBtmpIPaddr ) );
      AZX_LOG_DEBUG( "IP address: %s\r\n", CBtmpIPaddr);
      pdpEvent |= PDP_EVENT_UP;
      //azx_sleep_ms( 1000 );
      break;
    case M2MB_PDP_DOWN:
      AZX_LOG_DEBUG("PDP context deactivated\r\n");
      pdpEvent |= PDP_EVENT_DOWN;
      break;
    default:
      AZX_LOG_DEBUG("Unhandled PDP event: %d\r\n", pdp_event);
      break;
  }

  if(waitingPdpEvent)
  {
    m2mb_os_sem_put(pdpLockHandle);
  }
}

static BOOLEAN init_pdp_resources()
{
  if (pdpLockHandle == M2MB_OS_SEM_INVALID)
  {
    M2MB_OS_SEM_ATTR_HANDLE semaphore_attrs;
    m2mb_os_sem_setAttrItem(
        &semaphore_attrs, CMDS_ARGS(
          M2MB_OS_SEM_SEL_CMD_CREATE_ATTR, NULL,
          M2MB_OS_SEM_SEL_CMD_COUNT, 0 /*IPC*/,
          M2MB_OS_SEM_SEL_CMD_TYPE, M2MB_OS_SEM_BINARY,
          M2MB_OS_SEM_SEL_CMD_NAME, "pdpSem"));
    m2mb_os_sem_init(&pdpLockHandle, &semaphore_attrs);
  }

  if(pdpHandle == NULL)
  {
    if(M2MB_RESULT_SUCCESS != m2mb_pdp_init(&pdpHandle, pdp_cb, NULL))
    {
      AZX_LOG_ERROR("Unable to init m2mb_pdp API\r\n");
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN azx_connectivity_deactivatePdpSyncronously(UINT32 timeout_sec)
{
  M2MB_OS_RESULT_E rc;
  BOOLEAN result = FALSE;
  UINT8 status = 0xFF;

  init_pdp_resources();

  if(M2MB_RESULT_SUCCESS == m2mb_pdp_get_status(pdpHandle, AZX_PDP_CID, &status) &&
      status == 0)
  {
    AZX_LOG_DEBUG("PDP context already deactivated\r\n");
    return TRUE;
  }

  AZX_LOG_DEBUG("Waiting for PDP deactivation (timeout=%u s)\r\n", timeout_sec);

  if(M2MB_RESULT_SUCCESS != m2mb_pdp_deactivate(pdpHandle, AZX_PDP_CID))
  {
    AZX_LOG_ERROR("Unable to deactivate PDP\r\n");
    goto end;
  }

  pdpEvent = 0;
  waitingPdpEvent = TRUE;

  rc = m2mb_os_sem_get(pdpLockHandle, M2MB_OS_MS2TICKS(timeout_sec * 1000));

  if(rc != M2MB_OS_SUCCESS)
  {
    AZX_LOG_WARN("PDP deactivation has timed-out (err: %d)\r\n", rc);
    goto end;
  }

  if((pdpEvent & PDP_EVENT_DOWN) == 0)
  {
    AZX_LOG_WARN("Unexpected PDP event: %d\r\n", pdpEvent);
    goto end;
  }

  AZX_LOG_DEBUG("PDP deactivation confirmed\r\n");
  result = TRUE;

end:
  pdpEvent = 0;
  waitingPdpEvent = FALSE;
  return result;
}

static BOOLEAN wait_for_pdp(UINT32 timeout_ms)
{
  const struct ApnInfo* apn_info = azx_apn_getInfo();
  BOOLEAN result = FALSE;
  M2MB_OS_RESULT_E rc;

  AZX_LOG_DEBUG("Using APN %s\r\n", apn_info->apn);

  if(M2MB_RESULT_SUCCESS != m2mb_pdp_activate(pdpHandle, AZX_PDP_CID,
        (CHAR*)apn_info->apn, (CHAR*)apn_info->username, (CHAR*)apn_info->password,
        M2MB_PDP_IPV4))
  {
    AZX_LOG_ERROR("Unable to request PDP context activation.\r\n");
    goto end;
  }

  pdpEvent = 0;
  waitingPdpEvent = TRUE;

  rc = m2mb_os_sem_get(pdpLockHandle,
      (timeout_ms == TRY_FOREVER ? M2MB_OS_WAIT_FOREVER : M2MB_OS_MS2TICKS(timeout_ms)));

  if(rc != M2MB_OS_SUCCESS)
  {
    AZX_LOG_WARN("PDP activation has timed-out (err: %d)\r\n", rc);
    goto end;
  }

  if((pdpEvent & PDP_EVENT_UP) == 0)
  {
    if((pdpEvent & PDP_EVENT_DOWN) == 0)
    {
      AZX_LOG_WARN("Unexpected PDP event: %d\r\n", pdpEvent);
    }
    else
    {
      AZX_LOG_WARN("PDP activation failed\r\n");
    }
    goto end;
  }

  AZX_LOG_DEBUG("PDP activation confirmed\r\n");
  result = TRUE;

end:

  pdpEvent = 0;
  waitingPdpEvent = FALSE;
  return result;
}

BOOLEAN azx_connectivity_ConnectPdpSyncronously(UINT32 timeout_sec)
{
  UINT32 timeout_ms = timeout_sec * 1000;
  UINT8 status = 0xFF;

  AZX_LOG_DEBUG("Waiting for PDP context (timeout=%u s)\r\n", timeout_sec);

  init_pdp_resources();
  if(M2MB_RESULT_SUCCESS == m2mb_pdp_get_status(pdpHandle, AZX_PDP_CID, &status) &&
      status == 1)
  {
    AZX_LOG_DEBUG("PDP context already activated\r\n");
    return TRUE;
  }
  if(wait_for_pdp(timeout_ms))
  {
    /* Have a small delay to avoid trying to use the PDP context too quickly */
    azx_sleep_ms(2000);
    return TRUE;
  }

  return FALSE;
}

const AZX_CONNECTIVITY_INFO_T* azx_connectivity_getInfo()
{
  init_net_resources();

  connectivityInfo.status = M2MB_NET_STAT_UNKNOWN;
  connectivityInfo.rat = M2MB_NET_RAT_UNKNOWN;
  connectivityInfo.rssi = 99;
  connectivityInfo.ber = 99;

  wait_for_net_event(M2MB_NET_GET_REG_STATUS_INFO_RESP, FALSE);
  wait_for_net_event(M2MB_NET_GET_SIGNAL_INFO_RESP, FALSE);
  wait_for_net_event(M2MB_NET_GET_BER_RESP, FALSE);

  if(M2MB_RESULT_SUCCESS != m2mb_net_get_reg_status_info(netHandle))
  {
    AZX_LOG_WARN("Unable to get reg status\r\n");
    stop_waiting_for_net_event(M2MB_NET_GET_REG_STATUS_INFO_RESP, FALSE);
  }

  if(M2MB_RESULT_SUCCESS != m2mb_net_get_signal_info(netHandle))
  {
    AZX_LOG_WARN("Unable to get signal info\r\n");
    stop_waiting_for_net_event(M2MB_NET_GET_SIGNAL_INFO_RESP, FALSE);
  }

  if(M2MB_RESULT_SUCCESS != m2mb_net_get_ber(netHandle))
  {
    AZX_LOG_WARN("Unable to get ber info\r\n");
    stop_waiting_for_net_event(M2MB_NET_GET_BER_RESP, FALSE);
  }

  if(expectedNetEvents != 0)
  {
    M2MB_OS_RESULT_E rc = m2mb_os_sem_get(regLockHandle, INFO_GET_WAIT_MS);
    if(rc != M2MB_OS_SUCCESS)
    {
      AZX_LOG_WARN("Timeout waiting for info\r\n");
    }
  }

  stop_waiting_for_net_event(M2MB_NET_GET_REG_STATUS_INFO_RESP, FALSE);
  stop_waiting_for_net_event(M2MB_NET_GET_SIGNAL_INFO_RESP, FALSE);
  stop_waiting_for_net_event(M2MB_NET_GET_BER_RESP, FALSE);

  return &connectivityInfo;
}
