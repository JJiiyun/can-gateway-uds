#include "adas_signal.h"

#include <string.h>

void ADAS_Signal_BuildStatusFrame(const AdasStatus_t *status, CAN_Msg_t *out_msg)
{
    if (status == NULL || out_msg == NULL) {
        return;
    }

    memset(out_msg, 0, sizeof(*out_msg));
    out_msg->id = CAN_ID_ADAS_STATUS;
    out_msg->dlc = CAN_ADAS_STATUS_DLC;
    out_msg->data[CAN_ADAS_STATUS_FLAGS_IDX] = status->flags;
    out_msg->data[CAN_ADAS_STATUS_RISK_LEVEL_IDX] = status->risk_level;
    out_msg->data[CAN_ADAS_STATUS_FRONT_CM_IDX] = status->front_distance_cm;
    out_msg->data[CAN_ADAS_STATUS_REAR_CM_IDX] = status->rear_distance_cm;
    out_msg->data[CAN_ADAS_STATUS_FAULT_BITMAP_IDX] = status->fault_bitmap;
    out_msg->data[CAN_ADAS_STATUS_SPEED_KMH_IDX] = status->speed_kmh;
    out_msg->data[CAN_ADAS_STATUS_INPUT_BITMAP_IDX] = status->input_bitmap;
    out_msg->data[CAN_ADAS_STATUS_ALIVE_IDX] = status->alive;
}

void ADAS_Signal_DecodeStatusFrame(const CAN_Msg_t *msg, AdasStatus_t *out_status)
{
    if (msg == NULL || out_status == NULL ||
        msg->id != CAN_ID_ADAS_STATUS ||
        msg->dlc < CAN_ADAS_STATUS_DLC) {
        return;
    }

    out_status->flags = msg->data[CAN_ADAS_STATUS_FLAGS_IDX];
    out_status->risk_level = msg->data[CAN_ADAS_STATUS_RISK_LEVEL_IDX];
    out_status->front_distance_cm = msg->data[CAN_ADAS_STATUS_FRONT_CM_IDX];
    out_status->rear_distance_cm = msg->data[CAN_ADAS_STATUS_REAR_CM_IDX];
    out_status->fault_bitmap = msg->data[CAN_ADAS_STATUS_FAULT_BITMAP_IDX];
    out_status->speed_kmh = msg->data[CAN_ADAS_STATUS_SPEED_KMH_IDX];
    out_status->input_bitmap = msg->data[CAN_ADAS_STATUS_INPUT_BITMAP_IDX];
    out_status->alive = msg->data[CAN_ADAS_STATUS_ALIVE_IDX];
}
