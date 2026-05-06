#include "cmsis_os.h"
#include "can_bsp.h"
#include "can_cli_monitor.h"
#include "cli.h"
#include "gateway_router.h"
#include "gateway_safety_bridge.h"
#include "uart.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static volatile uint8_t s_gateway_ready = 0U;
static volatile uint8_t s_status_log_enabled = 1U;

static void log_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) {
        return;
    }
    if (len > (int)sizeof(buf)) {
        len = (int)sizeof(buf);
    }

    (void)uartWrite(0, (uint8_t *)buf, (uint32_t)len);
}

static void log_cmd(uint8_t argc, char *argv[])
{
    if (argc == 1U || strcmp(argv[1], "stat") == 0) {
        cliPrintf("status log: %s\r\n", s_status_log_enabled ? "on" : "off");
        return;
    }

    if (strcmp(argv[1], "on") == 0) {
        s_status_log_enabled = 1U;
        cliPrintf("status log on\r\n");
        return;
    }

    if (strcmp(argv[1], "off") == 0) {
        s_status_log_enabled = 0U;
        cliPrintf("status log off\r\n");
        return;
    }

    cliPrintf("usage: log [on|off|stat]\r\n");
}

void StartDefaultTask(void *argument)
{
    (void)argument;

    if (uartInit()) {
        cliInit();
        (void)cliAdd("log", log_cmd);
        CanCliMonitor_Init();
        cliPrintf("\r\n[GW] UART CLI ready. type 'help', 'log off', or 'canlog stat'\r\n");
    }

    if (CAN_BSP_Init() == HAL_OK) {
        GatewayRouter_Init();
        GatewaySafetyBridge_Init();
        s_gateway_ready = 1U;
        log_printf("\r\n[GW] Gateway init complete\r\n");
    } else {
        log_printf("\r\n[GW] Gateway init failed\r\n");
    }

    cliPrintf("CLI > ");

    for (;;) {
        cliMain();
        osDelay(1U);
    }
}

void GatewayTask(void *argument)
{
    (void)argument;

    while (s_gateway_ready == 0U) {
        osDelay(10U);
    }

    for (;;) {
        CAN_RxMessage_t rxMsg;

        if (!CAN_BSP_Read(&rxMsg, osWaitForever)) {
            osDelay(1U);
            continue;
        }

        CanCliMonitor_LogRx(&rxMsg);
        GatewayRouter_OnRx(&rxMsg);
        GatewaySafetyBridge_OnRx(&rxMsg);
    }
}

void ClusterTask(void *argument)
{
    (void)argument;

    while (s_gateway_ready == 0U) {
        osDelay(10U);
    }

    for (;;) {
        GatewaySafetyBridge_Task10ms();
        osDelay(10U);
    }
}

void LoggerTask(void *argument)
{
    (void)argument;

    while (s_gateway_ready == 0U) {
        osDelay(10U);
    }

    for (;;) {
        if (s_status_log_enabled != 0U) {
            GatewaySafetyDiagnostic_t safety;
            GatewayRouterStats_t router;

            GatewaySafetyBridge_GetDiagnostic(&safety);
            GatewayRouter_GetStats(&router);

            log_printf("[GW] RX1=%lu TX1=%lu RX2=%lu TX2=%lu busy=%lu err=%lu "
                       "route=%lu ok=%lu fail=%lu ignore=%lu "
                       "adas=%u risk=%u fault=0x%02X dtc=0x%02X "
                       "front=%u rear=%u speed=%u alive=%u\r\n",
                       (unsigned long)can1RxCount,
                       (unsigned long)can1TxCount,
                       (unsigned long)can2RxCount,
                       (unsigned long)can2TxCount,
                       (unsigned long)canTxBusyCount,
                       (unsigned long)canTxErrorCount,
                       (unsigned long)router.matched_count,
                       (unsigned long)router.tx_ok_count,
                       (unsigned long)router.tx_fail_count,
                       (unsigned long)router.ignored_count,
                       (unsigned int)safety.valid,
                       (unsigned int)safety.risk_level,
                       (unsigned int)safety.active_fault_bitmap,
                       (unsigned int)safety.dtc_bitmap,
                       (unsigned int)safety.front_distance_cm,
                       (unsigned int)safety.rear_distance_cm,
                       (unsigned int)safety.speed_kmh,
                       (unsigned int)safety.alive);
        }
        osDelay(1000U);
    }
}
