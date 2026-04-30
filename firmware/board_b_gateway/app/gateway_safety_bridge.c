#include "gateway_safety_bridge.h"

#include "can_cli_monitor.h"
#include "protocol_ids.h"

#include <stdbool.h>
#include <string.h>

extern CAN_HandleTypeDef hcan2;

#ifndef GATEWAY_SAFETY_WARNING_PERIOD_MS
#define GATEWAY_SAFETY_WARNING_PERIOD_MS 100U
#endif

#ifndef GATEWAY_SAFETY_TIMEOUT_MS
#define GATEWAY_SAFETY_TIMEOUT_MS 500U
#endif

/*
 * KMatrix maps CAN 0x3A0 to Golf6 mBremse_10 on the cluster bus. Keep
 * forwarding off by default so Board E's internal ADAS_Status does not collide
 * with a real Golf6 brake frame.
 */
#ifndef GATEWAY_SAFETY_FORWARD_ADAS_STATUS
#define GATEWAY_SAFETY_FORWARD_ADAS_STATUS 0
#endif

static GatewaySafetyDiagnostic_t s_diag;
static uint32_t s_next_warning_tick;
static uint8_t s_warning_alive;

static uint32_t tick_elapsed(uint32_t now, uint32_t before)
{
    return now - before;
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

static bool safety_warning_active(uint32_t now)
{
    if (!safety_status_recent(now)) {
        return false;
    }

    return s_diag.risk_level >= 2U ||
           (s_diag.flags & ADAS_FLAG_SENSOR_FAULT) != 0U ||
           s_diag.active_fault_bitmap != 0U;
}

static void build_motor5_warning(uint8_t data[8], uint32_t now)
{
    bool active = safety_warning_active(now);
    bool high_risk = active && s_diag.risk_level >= 3U;
    bool fault = active &&
                 ((s_diag.flags & ADAS_FLAG_SENSOR_FAULT) != 0U ||
                  s_diag.active_fault_bitmap != 0U);

    memset(data, 0, GOLF6_MOTOR_5_DLC);

    set_bit(data, GOLF6_MO5_HLEUCHTE_BIT, active ? 1U : 0U);
    set_bit(data, GOLF6_MO5_HEISSL_BIT, high_risk ? 1U : 0U);
    set_bit(data, GOLF6_MO5_TDE_LAMPE_BIT, fault ? 1U : 0U);
    set_bit(data, GOLF6_MO5_MOTORTEXT3_BIT, fault ? 1U : 0U);

    data[7] = s_warning_alive++;
}

static void send_warning(uint32_t now)
{
    uint8_t data[8];
    HAL_StatusTypeDef status;

    build_motor5_warning(data, now);
    status = CAN_BSP_SendTo(&hcan2, CAN_ID_WARNING, data, GOLF6_MOTOR_5_DLC);
    CanCliMonitor_LogTx(2U, CAN_ID_WARNING, data, GOLF6_MOTOR_5_DLC, status);
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
    s_next_warning_tick = 0U;
    s_warning_alive = 0U;
}

void GatewaySafetyBridge_OnRx(const CAN_RxMessage_t *rx_msg)
{
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

    if (safety_warning_active(s_diag.last_rx_tick)) {
        send_warning(s_diag.last_rx_tick);
        s_next_warning_tick = s_diag.last_rx_tick + GATEWAY_SAFETY_WARNING_PERIOD_MS;
    }
}

void GatewaySafetyBridge_Task10ms(void)
{
    uint32_t now = osKernelGetTickCount();

    if (s_next_warning_tick == 0U) {
        s_next_warning_tick = now;
    }

    if (tick_elapsed(now, s_next_warning_tick) < 0x80000000UL) {
        if (safety_status_recent(now)) {
            send_warning(now);
        }
        s_next_warning_tick = now + GATEWAY_SAFETY_WARNING_PERIOD_MS;
    }
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
