#include "gateway_safety_bridge.h"

#include "can_cli_monitor.h"
#include "protocol_ids.h"

#include <stdbool.h>
#include <string.h>

extern CAN_HandleTypeDef hcan2;

#ifndef GATEWAY_SAFETY_GONG_PULSE_MS
#define GATEWAY_SAFETY_GONG_PULSE_MS 100U
#endif

#ifndef GATEWAY_SAFETY_GONG_RISK2_PERIOD_MS
#define GATEWAY_SAFETY_GONG_RISK2_PERIOD_MS 600U
#endif

#ifndef GATEWAY_SAFETY_GONG_RISK3_PERIOD_MS
#define GATEWAY_SAFETY_GONG_RISK3_PERIOD_MS 300U
#endif

#ifndef GATEWAY_SAFETY_GONG_RISK4_PERIOD_MS
#define GATEWAY_SAFETY_GONG_RISK4_PERIOD_MS 150U
#endif

#ifndef GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_PERIOD_MS
#define GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_PERIOD_MS 10U
#endif

#ifndef GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK2_PERIOD_MS
#define GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK2_PERIOD_MS \
    GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_PERIOD_MS
#endif

#ifndef GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK3_PERIOD_MS
#define GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK3_PERIOD_MS 5U
#endif

#ifndef GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK4_PERIOD_MS
#define GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK4_PERIOD_MS 2U
#endif

#ifndef GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_MAX_BURST
#define GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_MAX_BURST 8U
#endif

#ifndef GATEWAY_SAFETY_TIMEOUT_MS
#define GATEWAY_SAFETY_TIMEOUT_MS 500U
#endif

/*
 * CAN_ID_ADAS_STATUS is a temporary internal Board E -> Gateway ID. The payload
 * layout follows Board E's ADAS Status frame; do not reuse Board A's 0x5A0
 * speed auxiliary ID for this path.
 */
#ifndef GATEWAY_SAFETY_FORWARD_ADAS_STATUS
#define GATEWAY_SAFETY_FORWARD_ADAS_STATUS 0
#endif

static GatewaySafetyDiagnostic_t s_diag;
static uint32_t s_next_gong_tick;
static uint32_t s_gong_clear_tick;
static uint32_t s_next_parking_assist_sweep_tick;
static uint8_t s_parking_assist_byte1_sweep;
static bool s_gong_asserted;
static bool s_parking_assist_was_active;

static uint32_t tick_elapsed(uint32_t now, uint32_t before)
{
    return now - before;
}

static bool tick_reached(uint32_t now, uint32_t target)
{
    return tick_elapsed(now, target) < 0x80000000UL;
}

static void set_bit(uint8_t data[8], uint8_t bit, uint8_t on)
{
    uint8_t mask = (uint8_t)(1U << (bit % 8U));

    if (on != 0U) {
        data[bit / 8U] |= mask;
    } else {
        data[bit / 8U] &= (uint8_t)~mask;
    }
}

static bool safety_status_recent(uint32_t now)
{
    if (s_diag.valid == 0U) {
        return false;
    }

    return tick_elapsed(now, s_diag.last_rx_tick) <= GATEWAY_SAFETY_TIMEOUT_MS;
}

static bool seatbelt_gong_active(uint32_t now)
{
    return safety_status_recent(now) && s_diag.risk_level >= 2U;
}

static uint32_t seatbelt_gong_period_ms(void)
{
    if (s_diag.risk_level >= 4U) {
        return GATEWAY_SAFETY_GONG_RISK4_PERIOD_MS;
    }

    if (s_diag.risk_level >= 3U) {
        return GATEWAY_SAFETY_GONG_RISK3_PERIOD_MS;
    }

    return GATEWAY_SAFETY_GONG_RISK2_PERIOD_MS;
}

static uint32_t parking_assist_sweep_period_ms(void)
{
    if (s_diag.risk_level >= 4U) {
        return GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK4_PERIOD_MS;
    }

    if (s_diag.risk_level >= 3U) {
        return GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK3_PERIOD_MS;
    }

    return GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_RISK2_PERIOD_MS;
}

static void build_parking_assist_gong(uint8_t data[8],
                                      bool gong_active,
                                      bool sweep_byte1)
{
    memset(data, 0, PARKING_ASSIST_DLC);
    set_bit(data, PARKING_ASSIST_GONG_BIT, gong_active ? 1U : 0U);

    if (sweep_byte1) {
        /* Probe unknown Parking Assist byte[1] payload: 00..FF continuously. */
        data[1] = s_parking_assist_byte1_sweep++;
    }
}

static void send_seatbelt_gong(bool gong_active, bool sweep_byte1)
{
    uint8_t data[8];
    HAL_StatusTypeDef status;

    build_parking_assist_gong(data, gong_active, sweep_byte1);
    status = CAN_BSP_SendTo(&hcan2, CAN_ID_PARKING_ASSIST, data, PARKING_ASSIST_DLC);
    CanCliMonitor_LogTx(2U, CAN_ID_PARKING_ASSIST, data, PARKING_ASSIST_DLC, status);
}

static void update_seatbelt_gong(uint32_t now)
{
    if (!seatbelt_gong_active(now)) {
        s_gong_asserted = false;
        s_next_gong_tick = 0U;
        s_gong_clear_tick = 0U;
        s_diag.gong_flags = 0U;
        return;
    }

    s_diag.gong_flags = s_diag.risk_level >= 3U ?
                        ADAS_GONG_DANGER :
                        ADAS_GONG_WARNING;

    if (s_next_gong_tick == 0U) {
        s_next_gong_tick = now;
    }

    if (s_gong_asserted && tick_reached(now, s_gong_clear_tick)) {
        s_gong_asserted = false;
    }

    if (!s_gong_asserted && tick_reached(now, s_next_gong_tick)) {
        s_gong_asserted = true;
        s_gong_clear_tick = now + GATEWAY_SAFETY_GONG_PULSE_MS;
        s_next_gong_tick = now + seatbelt_gong_period_ms();
    }
}

static void update_parking_assist_sweep(uint32_t now)
{
    uint32_t period_ms;
    uint8_t burst_count = 0U;

    if (!seatbelt_gong_active(now)) {
        if (s_parking_assist_was_active) {
            send_seatbelt_gong(false, false);
        }

        s_parking_assist_was_active = false;
        s_next_parking_assist_sweep_tick = 0U;
        s_parking_assist_byte1_sweep = 0U;
        return;
    }

    s_parking_assist_was_active = true;

    if (s_next_parking_assist_sweep_tick == 0U) {
        s_next_parking_assist_sweep_tick = now;
    }

    period_ms = parking_assist_sweep_period_ms();
    if (period_ms == 0U) {
        period_ms = 1U;
    }

    while (tick_reached(now, s_next_parking_assist_sweep_tick) &&
           burst_count < GATEWAY_SAFETY_PARKING_ASSIST_SWEEP_MAX_BURST) {
        send_seatbelt_gong(s_gong_asserted, true);
        s_next_parking_assist_sweep_tick += period_ms;
        burst_count++;
    }

    if (tick_reached(now, s_next_parking_assist_sweep_tick)) {
        s_next_parking_assist_sweep_tick = now + period_ms;
    }
}

static void forward_adas_status(const CAN_RxMessage_t *rx_msg)
{
#if GATEWAY_SAFETY_FORWARD_ADAS_STATUS
    HAL_StatusTypeDef status =
        CAN_BSP_SendTo(&hcan2, CAN_ID_ADAS_STATUS, (uint8_t *)rx_msg->data, rx_msg->dlc);
    CanCliMonitor_LogTx(2U, CAN_ID_ADAS_STATUS, rx_msg->data, rx_msg->dlc, status);
#else
    (void)rx_msg;
#endif
}

void GatewaySafetyBridge_Init(void)
{
    memset(&s_diag, 0, sizeof(s_diag));
    s_next_gong_tick = 0U;
    s_gong_clear_tick = 0U;
    s_next_parking_assist_sweep_tick = 0U;
    s_parking_assist_byte1_sweep = 0U;
    s_gong_asserted = false;
    s_parking_assist_was_active = false;
}

void GatewaySafetyBridge_OnRx(const CAN_RxMessage_t *rx_msg)
{
    uint8_t previous_risk_level = s_diag.risk_level;

    if (rx_msg == NULL ||
        rx_msg->bus != 1U ||
        rx_msg->id != CAN_ID_ADAS_STATUS ||
        rx_msg->dlc < CAN_ADAS_STATUS_DLC) {
        return;
    }

    s_diag.valid = 1U;
    s_diag.flags = rx_msg->data[CAN_ADAS_STATUS_FLAGS_IDX];
    s_diag.risk_level = rx_msg->data[CAN_ADAS_STATUS_RISK_LEVEL_IDX];
    s_diag.front_distance_cm = rx_msg->data[CAN_ADAS_STATUS_FRONT_CM_IDX];
    s_diag.rear_distance_cm = rx_msg->data[CAN_ADAS_STATUS_REAR_CM_IDX];
    s_diag.active_fault_bitmap = rx_msg->data[CAN_ADAS_STATUS_FAULT_BITMAP_IDX];
    s_diag.gong_flags = 0U;
    s_diag.speed_kmh = rx_msg->data[CAN_ADAS_STATUS_SPEED_KMH_IDX];
    s_diag.alive = rx_msg->data[CAN_ADAS_STATUS_ALIVE_IDX];
    s_diag.last_rx_tick = osKernelGetTickCount();

    if (s_diag.active_fault_bitmap != 0U ||
        (s_diag.flags & ADAS_FLAG_SENSOR_FAULT) != 0U) {
        s_diag.dtc_bitmap |= s_diag.active_fault_bitmap;
        if ((s_diag.flags & ADAS_FLAG_SENSOR_FAULT) != 0U) {
            s_diag.dtc_bitmap |= ADAS_FAULT_FRONT_SENSOR | ADAS_FAULT_REAR_SENSOR;
        }
    }

    forward_adas_status(rx_msg);

    if (s_diag.risk_level >= 2U &&
        (previous_risk_level != s_diag.risk_level || s_next_gong_tick == 0U)) {
        s_next_gong_tick = s_diag.last_rx_tick;
    }
}

void GatewaySafetyBridge_Task10ms(void)
{
    uint32_t now = osKernelGetTickCount();

    update_seatbelt_gong(now);
    update_parking_assist_sweep(now);
}

void GatewaySafetyBridge_GetDiagnostic(GatewaySafetyDiagnostic_t *out_diag)
{
    if (out_diag != NULL) {
        *out_diag = s_diag;
    }
}

void GatewaySafetyBridge_ClearDtc(void)
{
    s_diag.dtc_bitmap = 0U;
}
