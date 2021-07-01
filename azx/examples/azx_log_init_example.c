#include "m2mb_types.h"
#include "azx_log.h"
#include "app_cfg.h"

void M2MB_main( int argc, char **argv )
{

  AZX_LOG_CFG_T cfg = {
    .log_level=AZX_LOG_LEVEL_DEBUG,
    .log_channel=AZX_LOG_TO_MAIN_UART,
    .log_colours=0};
  azx_log_init(&cfg);

	//Print a message with INFO level
	AZX_LOG_INFO("Hello world!\n\n");
}