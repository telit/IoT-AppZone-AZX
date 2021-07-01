#include "m2mb_types.h"
#include "m2mb_sms.h"


#include "azx_log.h"
#include "azx_pduDec.h"

#include "app_cfg.h"

void M2MB_main( int argc, char **argv )
{

  //Demo: "how are you" sent by +391234567890
  uint8_t pdu[] = {0x07, 0x91, 0x93, 0x23, 0x50, 0x59, 0x12, 0x11, 0x00, 0x0C, 0x91, 0x93, 0x21,
  0x43, 0x65, 0x87, 0x09, 0x00, 0x00, 0x81, 0x01, 0x91, 0x11, 0x81, 0x91, 0x80,
  0x0C, 0xC8, 0xF7, 0x1D, 0x14, 0x96, 0x97, 0x41, 0xF9, 0x77, 0xFD, 0x07};
  uint32_t len = sizeof(pdu);

  //"how are you" sent by +391234567890
  char pdustr[] = "0791932350591211000C919321436587090000810191118191800CC8F71D14969741F977FD07";

  pdu_struct pack;
  char number[32];
  char msg[160];

  int len = azx_pdu_decode(pdu, len, &pack, number, msg);

  AZX_LOG_INFO("packet code type: %d\r\n", packet.tp_dcs);
  AZX_LOG_INFO("packet sender type: %u\r\n", packet.sender.type);
  AZX_LOG_INFO("packet msg len: %u\r\n", packet.msg.len);
  AZX_LOG_INFO("packet msg bytes: %u\r\n", packet.msg.bytes);
  AZX_LOG_INFO("packet protocol ID: %d\r\n", packet.tp_pid);

  AZX_LOG_INFO("packet date %u/%u/%u %u:%u:%u %.0f\r\n", packet.year, packet.month, packet.date, packet.hour, packet.min, packet.sec, packet.tz / 4.0);

  AZX_LOG_INFO("Received SMS, content: %s\r\n", message);
  AZX_LOG_INFO("Sender: %s\r\n", number);


  int len = azx_pdu_decodeString(pdustr, &pack, number, msg);

  AZX_LOG_INFO("packet code type: %d\r\n", packet.tp_dcs);
  AZX_LOG_INFO("packet sender type: %u\r\n", packet.sender.type);
  AZX_LOG_INFO("packet msg len: %u\r\n", packet.msg.len);
  AZX_LOG_INFO("packet msg bytes: %u\r\n", packet.msg.bytes);
  AZX_LOG_INFO("packet protocol ID: %d\r\n", packet.tp_pid);

  AZX_LOG_INFO("packet date %u/%u/%u %u:%u:%u %.0f\r\n", packet.year, packet.month, packet.date, packet.hour, packet.min, packet.sec, packet.tz / 4.0);

  AZX_LOG_INFO("Received SMS, content: %s\r\n", message);
  AZX_LOG_INFO("Sender: %s\r\n", number);


}