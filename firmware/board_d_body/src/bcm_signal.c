/**
 * @file    bcm_signal.c
 * @brief   BCM turn-signal CAN packing.
 */

#include "bcm_signal.h"
#include <string.h>

static void set_mask(uint8_t *byte, uint8_t mask, uint8_t on)
{
    if (on != 0U) {
        *byte |= mask;
    } else {
        *byte &= (uint8_t)~mask;
    }
}

static void init_frame(CAN_Msg_t *msg, uint32_t id, uint8_t dlc)
{
    memset(msg, 0, sizeof(*msg));
    msg->id = id;
    msg->dlc = dlc;
}

void BCM_Signal_BuildTurnFrame(const BcmSignal_TurnStatus_t *status,
                               CAN_Msg_t *out_msg)
{
    uint8_t left_on;
    uint8_t right_on;
    uint8_t hazard_on;

    if (status == NULL || out_msg == NULL) {
        return;
    }

    init_frame(out_msg, CAN_ID_CLUSTER_TURN_STATUS, CAN_CLUSTER_FRAME_DLC);

    left_on = ((status->input.turn_left_enabled ||
                status->input.hazard_enabled) &&
               status->left_blink_on);
    right_on = ((status->input.turn_right_enabled ||
                 status->input.hazard_enabled) &&
                status->right_blink_on);
    hazard_on = left_on && right_on;

    set_mask(&out_msg->data[CAN_CLUSTER_531_TURN_IDX],
             CLUSTER_531_TURN_LEFT_BIT,
             left_on);
    set_mask(&out_msg->data[CAN_CLUSTER_531_TURN_IDX],
             CLUSTER_531_TURN_RIGHT_BIT,
             right_on);
    set_mask(&out_msg->data[CAN_CLUSTER_531_TURN_IDX],
             CLUSTER_531_TURN_HAZARD_BIT,
             hazard_on);
}

void BCM_Signal_BuildBrightnessFrame(uint8_t level_percent,
                                     CAN_Msg_t *out_msg)
{
    if (out_msg == NULL) {
        return;
    }

    init_frame(out_msg, CAN_ID_CLUSTER_BRIGHTNESS, CAN_CLUSTER_BRIGHTNESS_DLC);
    if (level_percent > CAN_CLUSTER_BRIGHTNESS_MAX) {
        level_percent = CAN_CLUSTER_BRIGHTNESS_MAX;
    }

    out_msg->data[CAN_CLUSTER_BRIGHTNESS_LEVEL_IDX] = level_percent;
}
