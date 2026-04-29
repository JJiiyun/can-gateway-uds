#include "gateway_body_bridge.h"

#include "can_cli_monitor.h"

#include <stdbool.h>
#include <string.h>

extern CAN_HandleTypeDef hcan2;

#define BODY_STATUS_PERIOD_MS      100U
#define BODY_STATUS_TIMEOUT_MS     500U

typedef struct {
    uint8_t data[8];
    uint8_t dlc;
    uint32_t last_rx_tick;
    bool valid;
} BodyBridgeState_t;

static BodyBridgeState_t s_body_status = {0};
static uint32_t s_next_body_status_tick = 0U;

static uint32_t tick_elapsed(uint32_t now, uint32_t before)
{
    return now - before;
}

static void send_body_status(void)
{
    uint8_t data[8];

    if (!s_body_status.valid) {
        return;
    }

    memcpy(data, s_body_status.data, sizeof(data));
    HAL_StatusTypeDef status =
        CAN_BSP_SendTo(&hcan2, CAN_ID_BODY_STATUS, data, s_body_status.dlc);
    CanCliMonitor_LogTx(2U, CAN_ID_BODY_STATUS, data, s_body_status.dlc, status);
}

static bool is_body_status_recent(uint32_t now)
{
    if (!s_body_status.valid) {
        return false;
    }

    return tick_elapsed(now, s_body_status.last_rx_tick) <= BODY_STATUS_TIMEOUT_MS;
}

void GatewayBodyBridge_OnRx(const CAN_RxMessage_t *rx_msg)
{
    if (rx_msg == NULL) {
        return;
    }

    if (rx_msg->bus != 1U ||
        rx_msg->id != CAN_ID_BODY_STATUS ||
        rx_msg->dlc != CAN_DLC_BODY_STATUS) {
        return;
    }

    memcpy(s_body_status.data, rx_msg->data, sizeof(s_body_status.data));
    s_body_status.dlc = rx_msg->dlc;
    s_body_status.last_rx_tick = osKernelGetTickCount();
    s_body_status.valid = true;
    s_next_body_status_tick = s_body_status.last_rx_tick + BODY_STATUS_PERIOD_MS;

    send_body_status();
}

void GatewayBodyBridge_Task10ms(void)
{
    uint32_t now = osKernelGetTickCount();

    if (s_next_body_status_tick == 0U) {
        s_next_body_status_tick = now;
    }

    if (tick_elapsed(now, s_next_body_status_tick) < 0x80000000UL) {
        if (is_body_status_recent(now)) {
            send_body_status();
        }
        s_next_body_status_tick = now + BODY_STATUS_PERIOD_MS;
    }
}

uint8_t GatewayBodyBridge_HasRecentStatus(void)
{
    return is_body_status_recent(osKernelGetTickCount()) ? 1U : 0U;
}
