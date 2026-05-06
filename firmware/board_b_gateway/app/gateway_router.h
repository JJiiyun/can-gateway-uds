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

void GatewayRouter_Init(void);
void GatewayRouter_OnRx(const CAN_RxMessage_t *rx_msg);
void GatewayRouter_GetStats(GatewayRouterStats_t *out_stats);

#endif /* GATEWAY_ROUTER_H */
