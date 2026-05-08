#include "gateway_router.h"

#include "can_cli_monitor.h"
#include "protocol_ids.h"

#include <string.h>

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

#define CAN1_PORT 1U
#define CAN2_PORT 2U

typedef struct {
    uint32_t can_id;
    uint8_t src_bus;
    uint8_t dst_bus;
} GatewayRoute_t;

static const GatewayRoute_t s_route_table[] = {
    {CAN_ID_CLUSTER_IGN_STATUS, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_RPM, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_SPEED_1A0, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_SPEED_5A0, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_COOLANT, CAN1_PORT, CAN2_PORT},
    {CAN_ID_ENGINE_WARNING_STATUS, CAN1_PORT, CAN2_PORT},
    {CAN_ID_BODY_STATUS, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_TURN, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_BRIGHTNESS, CAN1_PORT, CAN2_PORT},
};

static GatewayRouterStats_t s_stats;
static GatewayRouterMonitor_t s_monitor;

static CAN_HandleTypeDef *bus_to_handle(uint8_t bus)
{
    if (bus == 1U) {
        return &hcan1;
    }
    if (bus == 2U) {
        return &hcan2;
    }
    return NULL;
}

static const GatewayRoute_t *find_route(const CAN_RxMessage_t *rx_msg)
{
    for (uint32_t i = 0U; i < (sizeof(s_route_table) / sizeof(s_route_table[0])); i++) {
        if (s_route_table[i].src_bus == rx_msg->bus &&
            s_route_table[i].can_id == rx_msg->id) {
            return &s_route_table[i];
        }
    }

    return NULL;
}

static uint16_t get_u16_le(const uint8_t *data, uint8_t idx)
{
    return (uint16_t)(((uint16_t)data[idx + 1U] << 8) | data[idx]);
}

static uint16_t clamp_speed(uint16_t speed)
{
    if (speed > CAN_CLUSTER_SPEED_MAX_KMH) {
        return CAN_CLUSTER_SPEED_MAX_KMH;
    }

    return speed;
}

static uint16_t decode_speed_5a0(uint8_t raw)
{
    return clamp_speed((uint16_t)raw * CAN_CLUSTER_SPEED_5A0_SCALE_MUL);
}

static uint8_t decode_coolant(uint8_t raw)
{
    uint32_t scaled = ((uint32_t)raw * CAN_CLUSTER_COOLANT_SCALE_NUM) /
                      CAN_CLUSTER_COOLANT_SCALE_DEN;

    if (scaled <= CAN_CLUSTER_COOLANT_OFFSET_C) {
        return 0U;
    }

    scaled -= CAN_CLUSTER_COOLANT_OFFSET_C;
    if (scaled > 255U) {
        return 255U;
    }

    return (uint8_t)scaled;
}

static void update_monitor(const CAN_RxMessage_t *rx_msg)
{
    uint16_t raw;

    if (rx_msg == NULL || rx_msg->bus != CAN1_PORT || rx_msg->dlc < 3U) {
        return;
    }

    switch (rx_msg->id) {
    case CAN_ID_CLUSTER_RPM:
        if (rx_msg->dlc >= 4U) {
            raw = get_u16_le(rx_msg->data, CAN_CLUSTER_RPM_RAW_IDX);
            s_monitor.rpm = (uint16_t)(raw / CAN_CLUSTER_RPM_SCALE_DIV);
            s_monitor.engine_valid = 1U;
        }
        break;

    case CAN_ID_CLUSTER_SPEED_1A0:
        if (rx_msg->dlc >= 4U) {
            raw = get_u16_le(rx_msg->data, CAN_CLUSTER_SPEED_1A0_RAW_IDX);
            s_monitor.speed_1a0 = clamp_speed((uint16_t)(raw / CAN_CLUSTER_SPEED_1A0_SCALE_DIV));
            s_monitor.engine_valid = 1U;
        }
        break;

    case CAN_ID_CLUSTER_SPEED_5A0:
        s_monitor.speed_5a0 = decode_speed_5a0(rx_msg->data[CAN_CLUSTER_SPEED_5A0_VALUE_IDX]);
        s_monitor.engine_valid = 1U;
        break;

    case CAN_ID_CLUSTER_COOLANT:
        if (rx_msg->dlc > CAN_CLUSTER_COOLANT_VALUE_IDX) {
            s_monitor.coolant = decode_coolant(rx_msg->data[CAN_CLUSTER_COOLANT_VALUE_IDX]);
            s_monitor.engine_valid = 1U;
        }
        break;

    case CAN_ID_CLUSTER_IGN_STATUS:
        if (rx_msg->dlc >= 6U) {
            s_monitor.ignition_on = (rx_msg->data[5] & 0x01U) != 0U ? 1U : 0U;
            s_monitor.engine_valid = 1U;
        }
        break;

    case CAN_ID_CLUSTER_TURN:
        s_monitor.turn_left = (rx_msg->data[2] & 0x01U) != 0U ? 1U : 0U;
        s_monitor.turn_right = (rx_msg->data[2] & 0x02U) != 0U ? 1U : 0U;
        s_monitor.hazard = (rx_msg->data[2] & 0x04U) != 0U ? 1U : 0U;
        s_monitor.turn_valid = 1U;
        break;

    default:
        break;
    }
}

void GatewayRouter_Init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
    memset(&s_monitor, 0, sizeof(s_monitor));
}

void GatewayRouter_OnRx(const CAN_RxMessage_t *rx_msg)
{
    const GatewayRoute_t *route;
    CAN_HandleTypeDef *dst_handle;
    uint8_t data[8];
    HAL_StatusTypeDef status;

    if (rx_msg == NULL || rx_msg->dlc > 8U) {
        s_stats.ignored_count++;
        return;
    }

    update_monitor(rx_msg);

    route = find_route(rx_msg);
    if (route == NULL) {
        s_stats.ignored_count++;
        return;
    }

    dst_handle = bus_to_handle(route->dst_bus);
    if (dst_handle == NULL) {
        s_stats.tx_fail_count++;
        return;
    }

    memcpy(data, rx_msg->data, rx_msg->dlc);

    s_stats.matched_count++;
    status = CAN_BSP_SendTo(dst_handle, rx_msg->id, data, rx_msg->dlc);
    CanCliMonitor_LogTx(route->dst_bus, rx_msg->id, data, rx_msg->dlc, status);

    if (status == HAL_OK) {
        s_stats.tx_ok_count++;
    } else {
        s_stats.tx_fail_count++;
    }
}

void GatewayRouter_GetStats(GatewayRouterStats_t *out_stats)
{
    if (out_stats != NULL) {
        *out_stats = s_stats;
    }
}

void GatewayRouter_GetMonitor(GatewayRouterMonitor_t *out_monitor)
{
    if (out_monitor != NULL) {
        *out_monitor = s_monitor;
    }
}
