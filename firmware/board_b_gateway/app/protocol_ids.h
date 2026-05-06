#ifndef PROTOCOL_IDS_H
#define PROTOCOL_IDS_H

#include <stdint.h>

#define CAN_ID_BODY_STATUS           0x390U
#define CAN_ID_ADAS_STATUS           0x3A0U
#define CAN_ID_WARNING               0x480U

#define CAN_ID_CLUSTER_SPEED_1A0     0x1A0U
#define CAN_ID_CLUSTER_RPM           0x280U
#define CAN_ID_CLUSTER_COOLANT       0x288U
#define CAN_ID_CLUSTER_TURN          0x531U
#define CAN_ID_CLUSTER_SPEED_5A0     0x5A0U
#define CAN_ID_CLUSTER_BRIGHTNESS    0x635U

#define CAN_CLUSTER_DLC                  8U
#define CAN_DLC_BODY_STATUS              8U
#define CAN_ADAS_STATUS_DLC              8U

#define CAN_CLUSTER_RPM_RAW_IDX          2U
#define CAN_CLUSTER_RPM_SCALE_DIV        4U
#define CAN_CLUSTER_SPEED_1A0_RAW_IDX    2U
#define CAN_CLUSTER_SPEED_1A0_SCALE_DIV  160U
#define CAN_CLUSTER_SPEED_5A0_VALUE_IDX  2U
#define CAN_CLUSTER_COOLANT_VALUE_IDX    1U

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

#endif
