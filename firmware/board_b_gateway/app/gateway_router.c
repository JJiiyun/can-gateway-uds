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
    {CAN_ID_CLUSTER_RPM, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_SPEED_1A0, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_SPEED_5A0, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_COOLANT, CAN1_PORT, CAN2_PORT},
    {CAN_ID_BODY_STATUS, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_TURN, CAN1_PORT, CAN2_PORT},
    {CAN_ID_CLUSTER_BRIGHTNESS, CAN1_PORT, CAN2_PORT},
};

static GatewayRouterStats_t s_stats;

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

void GatewayRouter_Init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
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
