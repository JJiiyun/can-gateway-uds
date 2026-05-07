#include "adas_can.h"

#include "can_bsp.h"
#include "can.h"
#include "protocol_ids.h"
#include "uart.h"

#include <string.h>

extern CAN_HandleTypeDef hcan1;

#define ADAS_ENGINE_TIMEOUT_MS 500U

#ifndef CAN_ID_CLUSTER_SPEED_1A0
#define CAN_ID_CLUSTER_SPEED_1A0        0x1A0U
#endif

#ifndef CAN_ID_CLUSTER_SPEED_5A0
#define CAN_ID_CLUSTER_SPEED_5A0        0x5A0U
#endif

#ifndef CAN_CLUSTER_DLC
#define CAN_CLUSTER_DLC                 8U
#endif

#ifndef CAN_CLUSTER_SPEED_1A0_RAW_IDX
#define CAN_CLUSTER_SPEED_1A0_RAW_IDX   2U
#endif

#ifndef CAN_CLUSTER_SPEED_1A0_SCALE_DIV
#define CAN_CLUSTER_SPEED_1A0_SCALE_DIV 160U
#endif

#ifndef CAN_CLUSTER_SPEED_5A0_VALUE_IDX
#define CAN_CLUSTER_SPEED_5A0_VALUE_IDX 2U
#endif

static volatile AdasCanStats_t s_stats;
static volatile uint32_t s_last_speed_rx_tick;
static volatile uint32_t s_last_ign_rx_tick;
static volatile uint8_t s_ign_on;

static uint8_t ign_status_frame_is_on(const CAN_RxMessage_t *rx)
{
    return VW300_GET_IGN_ON(rx->data) ? 1U : 0U;
}

static uint16_t decode_cluster_speed_1a0(const CAN_RxMessage_t *rx)
{
    uint16_t raw = CAN_GetU16LE(rx->data, CAN_CLUSTER_SPEED_1A0_RAW_IDX);

    return (uint16_t)(raw / CAN_CLUSTER_SPEED_1A0_SCALE_DIV);
}

static uint16_t decode_cluster_speed_5a0(const CAN_RxMessage_t *rx)
{
    return rx->data[CAN_CLUSTER_SPEED_5A0_VALUE_IDX];
}

static void update_vehicle_speed(uint16_t speed_kmh)
{
    s_stats.speed_kmh = speed_kmh;
    s_last_speed_rx_tick = HAL_GetTick();
}

static void update_ign_status(const CAN_RxMessage_t *rx)
{
    s_ign_on = ign_status_frame_is_on(rx);
    s_last_ign_rx_tick = HAL_GetTick();
    s_stats.engine_active = s_ign_on;

    if (s_ign_on == 0U) {
        s_stats.speed_kmh = 0U;
    }
}

static uint8_t ign_status_recent(uint32_t now)
{
    if (s_ign_on == 0U || s_last_ign_rx_tick == 0U) {
        return 0U;
    }

    return ((uint32_t)(now - s_last_ign_rx_tick) <= ADAS_ENGINE_TIMEOUT_MS) ? 1U : 0U;
}

int ADAS_Can_Init(void)
{
    memset((void *)&s_stats, 0, sizeof(s_stats));
    s_last_speed_rx_tick = 0U;
    s_last_ign_rx_tick = 0U;
    s_ign_on = 0U;

    if (CAN_BSP_Init() != HAL_OK) {
        uartPrintf(0, "[ADAS] CAN BSP init failed\r\n");
        return -1;
    }

    uartPrintf(0, "[ADAS] CAN1 ready, tx id=0x%03X\r\n", CAN_ID_ADAS_STATUS);
    return 0;
}

void ADAS_Can_PollRx(uint32_t timeout_ms)
{
    CAN_RxMessage_t rx;

    if (!CAN_BSP_Read(&rx, timeout_ms)) {
        return;
    }

    s_stats.rx_count++;

    if (rx.bus != 1U) {
        return;
    }

    if (rx.id == CAN_ID_IGN_STATUS &&
        rx.dlc >= CAN_ENGINE_DATA_DLC) {
        update_ign_status(&rx);
        return;
    }

    if (rx.id == CAN_ID_CLUSTER_SPEED_1A0 &&
        rx.dlc >= CAN_CLUSTER_DLC) {
        update_vehicle_speed(decode_cluster_speed_1a0(&rx));
        return;
    }

    if (rx.id == CAN_ID_CLUSTER_SPEED_5A0 &&
        rx.dlc >= CAN_CLUSTER_DLC) {
        update_vehicle_speed(decode_cluster_speed_5a0(&rx));
        return;
    }

}

uint16_t ADAS_Can_GetVehicleSpeedKmh(void)
{
    uint32_t now = HAL_GetTick();

    if (!ign_status_recent(now)) {
        s_stats.engine_active = 0U;
        return 0U;
    }

    s_stats.engine_active = 1U;

    if (s_last_speed_rx_tick == 0U ||
        (uint32_t)(now - s_last_speed_rx_tick) > ADAS_ENGINE_TIMEOUT_MS) {
        return 0U;
    }

    return s_stats.speed_kmh;
}

int ADAS_Can_SendStatus(const AdasStatus_t *status)
{
    CAN_Msg_t msg;
    HAL_StatusTypeDef tx_status;

    if (status == NULL) {
        s_stats.tx_error_count++;
        return -1;
    }

    ADAS_Signal_BuildStatusFrame(status, &msg);
    tx_status = CAN_BSP_SendTo(&hcan1, msg.id, msg.data, msg.dlc);
    if (tx_status != HAL_OK) {
        s_stats.tx_error_count++;
        uartPrintf(0, "[ADAS] TX fail id=0x%03lX st=%d err=%lu\r\n",
                   (unsigned long)msg.id,
                   (int)tx_status,
                   (unsigned long)s_stats.tx_error_count);
        return -1;
    }

    s_stats.tx_count++;
    return 0;
}

void ADAS_Can_GetStats(AdasCanStats_t *out_stats)
{
    if (out_stats != NULL) {
        *out_stats = s_stats;
    }
}
