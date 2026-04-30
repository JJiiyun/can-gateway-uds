#ifndef PROTOCOL_IDS_H
#define PROTOCOL_IDS_H

#include <stdint.h>

#define CAN_ID_ENGINE_DATA           0x100U
#define CAN_ID_DASHBOARD_CTRL        0x200U
#define CAN_ID_BODY_STATUS           0x390U
#define CAN_ID_ADAS_STATUS           0x3A0U
#define CAN_ID_WARNING               0x480U

#define CAN_ID_UDS_REQ_BOARD_B       0x714U
#define CAN_ID_UDS_RESP_BOARD_B      0x77EU
#define CAN_ID_UDS_REQ_OBD_STD       0x7E0U
#define CAN_ID_UDS_RESP_OBD_STD      0x7E8U

#define UDS_DID_VIN                  0xF190U
#define UDS_DID_RPM                  0xF40CU
#define UDS_DID_SPEED                0xF40DU
#define UDS_DID_TEMP                 0xF40EU
#define UDS_DID_ADAS_STATUS          0xF410U
#define UDS_DID_ADAS_FRONT_DISTANCE  0xF411U
#define UDS_DID_ADAS_REAR_DISTANCE   0xF412U
#define UDS_DID_ADAS_FAULT_BITMAP    0xF413U

#define CAN_ENGINE_DATA_DLC              8U
#define CAN_DLC_BODY_STATUS              8U
#define CAN_ADAS_STATUS_DLC              8U

#define CAN_ENGINE_DATA_RPM_IDX          0U
#define CAN_ENGINE_DATA_SPEED_IDX        2U
#define CAN_ENGINE_DATA_COOLANT_IDX      4U
#define CAN_ENGINE_DATA_STATUS_IDX       5U
#define CAN_ENGINE_DATA_RESERVED0_IDX    6U
#define CAN_ENGINE_DATA_RESERVED1_IDX    7U

#define CAN_ENGINE_STATUS_IGN_MASK       0x01U
#define CAN_ENGINE_STATUS_ALIVE_SHIFT    1U
#define CAN_ENGINE_STATUS_ALIVE_MASK     0xFEU

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

#define ADAS_FAULT_FRONT_SENSOR          (1U << 0)
#define ADAS_FAULT_REAR_SENSOR           (1U << 1)
#define ADAS_FAULT_BUTTON_STUCK          (1U << 2)
#define ADAS_FAULT_MESSAGE_TIMEOUT       (1U << 3)

/* Golf 6 KMatrix/DBC: CAN 0x480 is mMotor_5. */
#define GOLF6_MOTOR_5_DLC                8U
#define GOLF6_MO5_HEISSL_BIT             12U
#define GOLF6_MO5_HLEUCHTE_BIT           40U
#define GOLF6_MO5_TDE_LAMPE_BIT          44U
#define GOLF6_MO5_MOTORTEXT3_BIT         54U

static inline uint16_t CAN_GetU16LE(const uint8_t *buf, uint8_t index)
{
    return ((uint16_t)buf[index + 1U] << 8) | buf[index];
}

#endif
