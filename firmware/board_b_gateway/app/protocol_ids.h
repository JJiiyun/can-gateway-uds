#ifndef PROTOCOL_IDS_H
#define PROTOCOL_IDS_H

#include <stdint.h>

#define CAN_ID_CLUSTER_IGN_STATUS    0x100U
#define CAN_ID_BODY_STATUS           0x390U
#define CAN_ID_ADAS_STATUS           0x3A0U  /* Temporary Board E -> Gateway ID. */
#define CAN_ID_GOLF6_AIRBAG_1        0x050U
#define CAN_ID_PARKING_ASSIST        0x5D6U
#define CAN_ID_GATEWAY_UDS_REQ       0x714U
#define CAN_ID_GATEWAY_UDS_RESP      0x77EU

#define CAN_ID_CLUSTER_SPEED_1A0     0x1A0U
#define CAN_ID_CLUSTER_RPM           0x280U
#define CAN_ID_CLUSTER_COOLANT       0x288U
#define CAN_ID_ENGINE_WARNING_STATUS 0x481U
#define CAN_ID_CLUSTER_TURN          0x531U
#define CAN_ID_CLUSTER_SPEED_5A0     0x5A0U
#define CAN_ID_CLUSTER_BRIGHTNESS    0x635U

#define CAN_CLUSTER_DLC                  8U
#define CAN_DLC_BODY_STATUS              8U
#define CAN_ADAS_STATUS_DLC              8U
#define CAN_ENGINE_WARNING_DLC           8U

#define CAN_CLUSTER_RPM_RAW_IDX          2U
#define CAN_CLUSTER_RPM_SCALE_DIV        4U
#define CAN_CLUSTER_SPEED_1A0_RAW_IDX    2U
#define CAN_CLUSTER_SPEED_1A0_SCALE_DIV  80U
#define CAN_CLUSTER_SPEED_MAX_KMH        240U
#define CAN_CLUSTER_SPEED_5A0_VALUE_IDX  2U
#define CAN_CLUSTER_SPEED_5A0_SCALE_MUL  2U
#define CAN_CLUSTER_COOLANT_VALUE_IDX    1U
#define CAN_CLUSTER_COOLANT_SCALE_NUM    3U
#define CAN_CLUSTER_COOLANT_SCALE_DEN    4U
#define CAN_CLUSTER_COOLANT_OFFSET_C     48U

#define CAN_ADAS_STATUS_FLAGS_IDX        0U
#define CAN_ADAS_STATUS_RISK_LEVEL_IDX   1U
#define CAN_ADAS_STATUS_FRONT_CM_IDX     2U
#define CAN_ADAS_STATUS_REAR_CM_IDX      3U
#define CAN_ADAS_STATUS_FAULT_BITMAP_IDX 4U
#define CAN_ADAS_STATUS_SPEED_KMH_IDX    5U
#define CAN_ADAS_STATUS_INPUT_BITMAP_IDX 6U
#define CAN_ADAS_STATUS_ALIVE_IDX        7U

#define ADAS_FLAG_FRONT_COLLISION        (1U << 0)
#define ADAS_FLAG_LANE_DEPARTURE         (1U << 1)
#define ADAS_FLAG_HARSH_BRAKE            (1U << 2)
#define ADAS_FLAG_REAR_OBSTACLE          (1U << 3)
#define ADAS_FLAG_SENSOR_FAULT           (1U << 4)
#define ADAS_FLAG_ACTIVE                 (1U << 5)

#define ADAS_GONG_WARNING                (1U << 4)
#define ADAS_GONG_DANGER                 (1U << 5)

#define ADAS_FAULT_FRONT_SENSOR          (1U << 0)
#define ADAS_FAULT_REAR_SENSOR           (1U << 1)
#define ADAS_FAULT_BUTTON_STUCK          (1U << 2)
#define ADAS_FAULT_MESSAGE_TIMEOUT       (1U << 3)

#define UDS_SID_CLEAR_DTC                 0x14U
#define UDS_SID_READ_DID                  0x22U
#define UDS_POS_CLEAR_DTC                 0x54U
#define UDS_POS_READ_DID                  0x62U
#define UDS_NEG_RESP                      0x7FU
#define UDS_NRC_SERVICE_NOT_SUPPORTED     0x11U
#define UDS_NRC_INCORRECT_LENGTH          0x13U
#define UDS_NRC_REQUEST_OUT_OF_RANGE      0x31U

#define UDS_DID_ADAS_STATUS               0xF410U
#define UDS_DID_ADAS_FRONT_DISTANCE       0xF411U
#define UDS_DID_ADAS_REAR_DISTANCE        0xF412U
#define UDS_DID_ADAS_FAULT_BITMAP         0xF413U

/* Parking Assist CAN 0x5D6 for ADAS warning gong. */
#define PARKING_ASSIST_DLC               8U
#define PARKING_ASSIST_GONG_BIT          0U

#endif
