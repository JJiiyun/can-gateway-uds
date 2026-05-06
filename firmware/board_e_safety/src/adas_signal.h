#ifndef ADAS_SIGNAL_H
#define ADAS_SIGNAL_H

#include "protocol_ids.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t flags;
    uint8_t risk_level;
    uint8_t front_distance_cm;
    uint8_t rear_distance_cm;
    uint8_t fault_bitmap;
    uint8_t speed_kmh;
    uint8_t input_bitmap;
    uint8_t alive;
} AdasStatus_t;

#ifndef CAN_ID_ADAS_STATUS
#define CAN_ID_ADAS_STATUS               0x3A0U
#endif

#ifndef CAN_ADAS_STATUS_DLC
#define CAN_ADAS_STATUS_DLC              8U
#endif

void ADAS_Signal_BuildStatusFrame(const AdasStatus_t *status, CAN_Msg_t *out_msg);
void ADAS_Signal_DecodeStatusFrame(const CAN_Msg_t *msg, AdasStatus_t *out_status);

#ifdef __cplusplus
}
#endif

#endif /* ADAS_SIGNAL_H */
