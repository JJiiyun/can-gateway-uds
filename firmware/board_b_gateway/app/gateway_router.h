#ifndef GATEWAY_ROUTER_H
#define GATEWAY_ROUTER_H

#include "can_bsp.h"

#include <stdint.h>

typedef struct {
    uint32_t matched_count;
    uint32_t tx_ok_count;
    uint32_t tx_fail_count;
    uint32_t ignored_count;
} GatewayRouterStats_t;

typedef struct {
    uint8_t engine_valid;
    uint16_t rpm;
    uint16_t speed_1a0;
    uint16_t speed_5a0;
    uint8_t coolant;
    uint8_t ignition_on;
    uint8_t engine_rpm_warning;
    uint8_t engine_coolant_warning;
    uint8_t engine_general_warning;
    uint8_t turn_valid;
    uint8_t turn_left;
    uint8_t turn_right;
    uint8_t hazard;
} GatewayRouterMonitor_t;

void GatewayRouter_Init(void);
void GatewayRouter_OnRx(const CAN_RxMessage_t *rx_msg);
void GatewayRouter_GetStats(GatewayRouterStats_t *out_stats);
void GatewayRouter_GetMonitor(GatewayRouterMonitor_t *out_monitor);

#endif /* GATEWAY_ROUTER_H */
