#ifndef PROTOCOL_IDS_H
#define PROTOCOL_IDS_H

#include "signal_db.h"

#define CAN_ID_ENGINE_DATA          0x100u
#define CAN_ID_DASHBOARD_CTRL       0x200u
#define CAN_ID_WARNING              0x480u
#define CAN_ID_BODY_STATUS          0x470u

#define CAN_ID_UDS_REQ_BOARD_B      0x714u
#define CAN_ID_UDS_RESP_BOARD_B     0x77Eu

#define UDS_DID_VIN                 0xF190u
#define UDS_DID_RPM                 0xF40Cu
#define UDS_DID_SPEED               0xF40Du
#define UDS_DID_TEMP                0xF40Eu

#define CAN_ENGINE_DATA_DLC         8u
#define CAN_DLC_BODY_STATUS         8u

#define BODY_BIT_TURN_LEFT          (1u << 0)
#define BODY_BIT_TURN_RIGHT         (1u << 1)
#define BODY_BIT_HIGH_BEAM          (1u << 2)
#define BODY_BIT_FOG_LAMP           (1u << 3)
#define BODY_BIT_DOOR_FL            (1u << 4)
#define BODY_BIT_DOOR_FR            (1u << 5)
#define BODY_BIT_DOOR_RL            (1u << 6)
#define BODY_BIT_DOOR_RR            (1u << 7)

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} CAN_Msg_t;

#define CAN_ENGINE_DATA_RPM_IDX          0u
#define CAN_ENGINE_DATA_SPEED_IDX        2u
#define CAN_ENGINE_DATA_COOLANT_IDX      4u
#define CAN_ENGINE_DATA_STATUS_IDX       5u
#define CAN_ENGINE_DATA_RESERVED0_IDX    6u
#define CAN_ENGINE_DATA_RESERVED1_IDX    7u

#define CAN_ENGINE_STATUS_IGN_MASK       0x01u
#define CAN_ENGINE_STATUS_ALIVE_SHIFT    1u
#define CAN_ENGINE_STATUS_ALIVE_MASK     0xFEu

#define CAN_ID_IGN_STATUS               CAN_ID_ENGINE_DATA

#define VW300_GET_IGN_ON(data_)      (((data_)[CAN_ENGINE_DATA_STATUS_IDX] & CAN_ENGINE_STATUS_IGN_MASK) != 0u)

#define VW470_SET_BIT(data_, bit_, on_) \
    do { \
        if ((on_) != 0u) { \
            (data_)[0] |= (uint8_t)(bit_); \
        } else { \
            (data_)[0] &= (uint8_t)~(bit_); \
        } \
    } while (0)

#define VW470_SET_TURN_LEFT(data_, on_)   VW470_SET_BIT((data_), BODY_BIT_TURN_LEFT, (on_))
#define VW470_SET_TURN_RIGHT(data_, on_)  VW470_SET_BIT((data_), BODY_BIT_TURN_RIGHT, (on_))
#define VW470_SET_HIGH_BEAM(data_, on_)   VW470_SET_BIT((data_), BODY_BIT_HIGH_BEAM, (on_))
#define VW470_SET_FOG_LIGHT(data_, on_)   VW470_SET_BIT((data_), BODY_BIT_FOG_LAMP, (on_))
#define VW470_SET_DOOR_FL(data_, on_)     VW470_SET_BIT((data_), BODY_BIT_DOOR_FL, (on_))
#define VW470_SET_DOOR_FR(data_, on_)     VW470_SET_BIT((data_), BODY_BIT_DOOR_FR, (on_))
#define VW470_SET_DOOR_RL(data_, on_)     VW470_SET_BIT((data_), BODY_BIT_DOOR_RL, (on_))
#define VW470_SET_DOOR_RR(data_, on_)     VW470_SET_BIT((data_), BODY_BIT_DOOR_RR, (on_))

#endif
