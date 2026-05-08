#ifndef ADAS_CAN_H
#define ADAS_CAN_H

#include "adas_signal.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t tx_error_count;
    uint32_t status_tx_period_ms;
    uint32_t status_tx_age_ms;
    uint32_t rx_period_ms;
    uint32_t rx_age_ms;
    uint32_t ign_rx_period_ms;
    uint32_t ign_rx_age_ms;
    uint32_t speed_rx_period_ms;
    uint32_t speed_rx_age_ms;
    uint16_t speed_kmh;
    uint8_t engine_active;
} AdasCanStats_t;

int ADAS_Can_Init(void);
void ADAS_Can_PollRx(uint32_t timeout_ms);
uint16_t ADAS_Can_GetVehicleSpeedKmh(void);
int ADAS_Can_SendStatus(const AdasStatus_t *status);
void ADAS_Can_GetStats(AdasCanStats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* ADAS_CAN_H */
