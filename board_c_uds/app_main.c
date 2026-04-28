#include "app_main.h"
#include "uds_server.h"
#include "cli_cmd.h"

void App_Init(void)
{
    UDS_Server_Init();
    UDS_CLI_Init();
}

void App_Run(void)
{
    // 필요 시 주기 처리
}