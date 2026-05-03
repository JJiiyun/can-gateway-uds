/**
 * @file    bcm_input.h
 * @brief   BCM DIP switch and button input handling.
 */

#ifndef BCM_INPUT_H
#define BCM_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BCM_INPUT_MODE_GPIO = 0,
    BCM_INPUT_MODE_UART = 1,
} BcmInput_Mode_t;

typedef enum {
    BCM_INPUT_FIELD_DOOR_FL = 0,
    BCM_INPUT_FIELD_DOOR_FR,
    BCM_INPUT_FIELD_DOOR_RL,
    BCM_INPUT_FIELD_DOOR_RR,
    BCM_INPUT_FIELD_TURN_LEFT,
    BCM_INPUT_FIELD_TURN_RIGHT,
    BCM_INPUT_FIELD_HIGH_BEAM,
    BCM_INPUT_FIELD_FOG_LIGHT,
} BcmInput_Field_t;

typedef struct {
    uint8_t door_fl;
    uint8_t door_fr;
    uint8_t door_rl;
    uint8_t door_rr;
    uint8_t turn_left_enabled;
    uint8_t turn_right_enabled;
    uint8_t high_beam;
    uint8_t fog_light;
} BcmInput_State_t;

void BCM_Input_Init(void);
void BCM_Input_Poll(void);
void BCM_Input_GetState(BcmInput_State_t *out_state);
void BCM_Input_SetState(const BcmInput_State_t *state);
void BCM_Input_SetField(BcmInput_Field_t field, uint8_t active);
void BCM_Input_SetAllDoors(uint8_t active);
void BCM_Input_SetAllLamps(uint8_t active);
void BCM_Input_ClearAll(void);
void BCM_Input_SetMode(BcmInput_Mode_t mode);
BcmInput_Mode_t BCM_Input_GetMode(void);
const char *BCM_Input_GetModeString(void);

#ifdef __cplusplus
}
#endif

#endif /* BCM_INPUT_H */
