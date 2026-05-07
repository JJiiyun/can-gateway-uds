#ifndef GATEWAY_SAFETY_BRIDGE_H
#define GATEWAY_SAFETY_BRIDGE_H

#include "can_bsp.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t valid;
    uint8_t flags;
    uint8_t risk_level;
    uint8_t front_distance_cm;
    uint8_t rear_distance_cm;
    uint8_t active_fault_bitmap;
    uint8_t dtc_bitmap;
    uint8_t gong_flags;
    uint8_t speed_kmh;
    uint8_t alive;
    uint32_t last_rx_tick;
} GatewaySafetyDiagnostic_t;

void GatewaySafetyBridge_Init(void);
void GatewaySafetyBridge_OnRx(const CAN_RxMessage_t *rx_msg);
void GatewaySafetyBridge_Task10ms(void);
void GatewaySafetyBridge_GetDiagnostic(GatewaySafetyDiagnostic_t *out_diag);
void GatewaySafetyBridge_ClearDtc(void);

#ifdef __cplusplus
}
#endif

#endif /* GATEWAY_SAFETY_BRIDGE_H */
