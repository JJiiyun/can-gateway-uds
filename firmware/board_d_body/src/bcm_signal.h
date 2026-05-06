/**
 * @file    bcm_signal.h
 * @brief   BCM turn-signal CAN packing.
 */

#ifndef BCM_SIGNAL_H
#define BCM_SIGNAL_H

#include <stdint.h>
#include "bcm_input.h"
#include "protocol_ids.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CAN_ID_CLUSTER_TURN
#define CAN_ID_CLUSTER_TURN           0x531U
#endif

#ifndef CAN_ID_CLUSTER_TURN_STATUS
#define CAN_ID_CLUSTER_TURN_STATUS    CAN_ID_CLUSTER_TURN
#endif

#ifndef CAN_CLUSTER_DLC
#define CAN_CLUSTER_DLC               8U
#endif

#ifndef CAN_CLUSTER_FRAME_DLC
#define CAN_CLUSTER_FRAME_DLC         CAN_CLUSTER_DLC
#endif

#ifndef CAN_CLUSTER_531_TURN_IDX
#define CAN_CLUSTER_531_TURN_IDX      2U
#endif

#ifndef CLUSTER_531_TURN_LEFT_BIT
#define CLUSTER_531_TURN_LEFT_BIT     (1U << 0)
#endif

#ifndef CLUSTER_531_TURN_RIGHT_BIT
#define CLUSTER_531_TURN_RIGHT_BIT    (1U << 1)
#endif

#ifndef CLUSTER_531_TURN_HAZARD_BIT
#define CLUSTER_531_TURN_HAZARD_BIT   (1U << 2)
#endif

typedef struct {
    BcmInput_State_t input;
    uint8_t left_blink_on;
    uint8_t right_blink_on;
} BcmSignal_TurnStatus_t;

void BCM_Signal_BuildTurnFrame(const BcmSignal_TurnStatus_t *status,
                               CAN_Msg_t *out_msg);

#ifdef __cplusplus
}
#endif

#endif /* BCM_SIGNAL_H */
