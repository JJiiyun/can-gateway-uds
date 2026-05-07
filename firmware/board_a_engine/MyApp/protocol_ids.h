#ifndef PROTOCOL_IDS_H
#define PROTOCOL_IDS_H

#include <stdint.h>

/* ============================================================
 * Cluster CAN IDs
 * Board A EngineSim -> CAN Bus
 * ============================================================ */

#define CAN_ID_CLUSTER_IGN_STATUS       0x100U
#define CAN_ID_CLUSTER_RPM              0x280U
#define CAN_ID_CLUSTER_SPEED_1A0        0x1A0U
#define CAN_ID_CLUSTER_SPEED_5A0        0x5A0U
#define CAN_ID_CLUSTER_COOLANT          0x288U

#define CAN_CLUSTER_DLC                 8U

/* ============================================================
 * 0x100 IGN Status Frame
 *
 * ID    : 0x100
 * DLC   : 8
 * byte5 : bit0 = IGN ON
 * ============================================================ */

#define CAN_IGN_STATUS_IDX              5U
#define CAN_IGN_STATUS_IGN_ON_MASK      0x01U

/* ============================================================
 * 0x280 RPM Frame
 *
 * Example:
 * ID   : 0x280
 * DLC  : 8
 * DATA : 00 00 15 15 00 00 00 00
 *
 * byte2~3 : RPM raw value
 * ============================================================ */

#define CAN_RPM_RAW_L_IDX               2U
#define CAN_RPM_RAW_H_IDX               3U

/* ============================================================
 * 0x1A0 Speed Frame
 *
 * Example:
 * ID   : 0x1A0
 * DLC  : 8
 * DATA : 08 00 20 4E 00 00 00 00
 *
 * byte2~3 : Speed raw value
 * byte0   : fixed value 0x08
 * ============================================================ */

#define CAN_SPEED_1A0_FIXED_B0          0x08U
#define CAN_SPEED_1A0_RAW_L_IDX         2U
#define CAN_SPEED_1A0_RAW_H_IDX         3U

/* ============================================================
 * 0x5A0 Speed Frame
 *
 * byte2 : Speed needle value
 * ============================================================ */

#define CAN_SPEED_5A0_VALUE_IDX         2U

/* ============================================================
 * 0x288 Coolant Frame
 *
 * byte1 : Coolant needle value
 * ============================================================ */

#define CAN_COOLANT_VALUE_IDX           1U

/* ============================================================
 * UDS Diagnostic IDs
 * Board B <-> Board C
 * ============================================================ */

#define CAN_ID_UDS_REQ_BOARD_B          0x7E0U
#define CAN_ID_UDS_RESP_BOARD_B         0x7E8U

/* ============================================================
 * UDS DID
 * ============================================================ */

#define UDS_DID_VIN                     0xF190U
#define UDS_DID_RPM                     0xF40CU
#define UDS_DID_SPEED                   0xF40DU
#define UDS_DID_TEMP                    0xF40EU

#endif
