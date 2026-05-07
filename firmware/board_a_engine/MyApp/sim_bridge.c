#include "sim_bridge.h"
#include "can_bsp.h"
#include "protocol_ids.h"
#include "uart.h"
#include "cli.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdlib.h>

#define SIM_UART_BUF_SIZE   128U
#define SIM_TIMEOUT_MS      3000U

static char     line_buf[SIM_UART_BUF_SIZE];
static uint16_t line_idx = 0U;

static SimBridgeData_t g_data         = {0};
static uint32_t        g_last_rx_tick = 0U;

/* ── CAN 송신 함수 ──────────────────────────────── */
static void SimBridge_SendRpm(uint16_t rpm)
{
    uint8_t data[8] = {0x00, 0x00, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00};
    uint16_t rpm_raw = rpm * 4U;
    data[CAN_RPM_RAW_L_IDX]      = (uint8_t)(rpm_raw & 0xFFU);
    data[CAN_RPM_RAW_L_IDX + 1U] = (uint8_t)(rpm_raw >> 8U);
    CAN_BSP_Send(CAN_ID_CLUSTER_RPM, data, 8U);
}

static void SimBridge_SendSpeed(uint16_t speed)
{
    // 0x1A0
    uint8_t data1a0[8] = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint16_t speed_raw = speed * 160U;
    data1a0[CAN_SPEED_1A0_RAW_L_IDX]      = (uint8_t)(speed_raw & 0xFFU);
    data1a0[CAN_SPEED_1A0_RAW_L_IDX + 1U] = (uint8_t)(speed_raw >> 8U);
    CAN_BSP_Send(CAN_ID_CLUSTER_SPEED_1A0, data1a0, 8U);

    // 0x5A0
    uint8_t data5a0[8] = {0};
    data5a0[CAN_SPEED_5A0_VALUE_IDX] = (uint8_t)(speed > 255U ? 255U : speed);
    CAN_BSP_Send(CAN_ID_CLUSTER_SPEED_5A0, data5a0, 8U);
}

/* ── 한 줄 파싱 ─────────────────────────────────── */
static void parse_line(const char *line)
{
    char buf[SIM_UART_BUF_SIZE];
    strncpy(buf, line, sizeof(buf) - 1U);
    buf[sizeof(buf) - 1U] = '\0';

    float rpm   = 0.0f;
    float speed = 0.0f;

    char *tok = strtok(buf, ",");
    while (tok != NULL)
    {
        char *colon = strchr(tok, ':');
        if (colon != NULL)
        {
            *colon = '\0';
            const char *key = tok;
            float val = strtof(colon + 1, NULL);

            if      (strcmp(key, "RPM")   == 0) rpm   = val;
            else if (strcmp(key, "SPEED") == 0) speed = val;
        }
        tok = strtok(NULL, ",");
    }

    g_data.rpm     = rpm;
    g_data.speed   = speed;
    g_data.valid   = true;
    g_last_rx_tick = osKernelGetTickCount();

    SimBridge_SendRpm((uint16_t)rpm);
    SimBridge_SendSpeed((uint16_t)speed);
}

/* ── 초기화 ─────────────────────────────────────── */
void SimBridge_Init(void)
{
    line_idx = 0U;
    memset(line_buf, 0, sizeof(line_buf));
    memset(&g_data, 0, sizeof(g_data));
    g_last_rx_tick = 0U;
}

/* ── Task ───────────────────────────────────────── */
void SimBridge_Task(void *argument)
{
    (void)argument;

    SimBridge_Init();

    for (;;)
    {
        uint8_t byte;
        if (uartReadBlock(0U, &byte, 10U) == true)
        {
            if (byte == '\n')
            {
                line_buf[line_idx] = '\0';
                if (line_idx > 0U)
                {
                    if (strncmp(line_buf, "RPM:", 4U) == 0)
                    {
                        parse_line(line_buf);
                    }
                    else
                    {
                        cliParseArgs(line_buf);
                        cliRunCommand();
                    }
                }
                line_idx = 0U;
            }
            else if (byte != '\r')
            {
                if (line_idx < SIM_UART_BUF_SIZE - 1U)
                    line_buf[line_idx++] = (char)byte;
                else
                    line_idx = 0U;
            }
        }

        osDelay(1U);
    }
}

/* ── 외부에서 데이터 읽기 ───────────────────────── */
void SimBridge_GetData(SimBridgeData_t *out)
{
    if (out != NULL)
        *out = g_data;
}

bool SimBridge_IsConnected(void)
{
    if (!g_data.valid) return false;
    return (osKernelGetTickCount() - g_last_rx_tick) < SIM_TIMEOUT_MS;
}