#include "adas_can.h"

#include "can_bsp.h"
#include "can.h"
#include "protocol_ids.h"
#include "uart.h"

#include <string.h>

extern CAN_HandleTypeDef hcan1;

#define ADAS_ENGINE_TIMEOUT_MS 500U

static volatile AdasCanStats_t s_stats;
static volatile uint32_t s_last_engine_rx_tick;

static uint8_t engine_frame_is_ign_on(const CAN_RxMessage_t *rx)
{
    return (rx->data[CAN_ENGINE_DATA_STATUS_IDX] & CAN_ENGINE_STATUS_IGN_MASK) != 0U;
}

int ADAS_Can_Init(void)
{
    memset((void *)&s_stats, 0, sizeof(s_stats));
    s_last_engine_rx_tick = 0U;

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

    if (rx.bus == 1U &&
        rx.id == CAN_ID_ENGINE_DATA &&
        rx.dlc >= CAN_ENGINE_DATA_DLC &&
        engine_frame_is_ign_on(&rx)) {
        s_stats.speed_kmh = CAN_GetU16LE(rx.data, CAN_ENGINE_DATA_SPEED_IDX);
        s_stats.engine_active = 1U;
        s_last_engine_rx_tick = HAL_GetTick();
    }
}

uint16_t ADAS_Can_GetVehicleSpeedKmh(void)
{
    if (!s_stats.engine_active) {
        return 0U;
    }

    if ((uint32_t)(HAL_GetTick() - s_last_engine_rx_tick) > ADAS_ENGINE_TIMEOUT_MS) {
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
