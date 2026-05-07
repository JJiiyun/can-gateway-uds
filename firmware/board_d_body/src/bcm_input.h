/**
 * @file    bcm_input.h
 * @brief   BCM turn-signal button input handling.
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
    BCM_INPUT_FIELD_TURN_LEFT = 0,
    BCM_INPUT_FIELD_TURN_RIGHT,
    BCM_INPUT_FIELD_HAZARD,
} BcmInput_Field_t;

typedef struct {
    uint8_t turn_left_enabled;
    uint8_t turn_right_enabled;
    uint8_t hazard_enabled;
} BcmInput_State_t;

void BCM_Input_Init(void);
void BCM_Input_Poll(void);
void BCM_Input_GetState(BcmInput_State_t *out_state);
void BCM_Input_GetRawState(BcmInput_State_t *out_raw_state);
void BCM_Input_SetState(const BcmInput_State_t *state);
void BCM_Input_SetField(BcmInput_Field_t field, uint8_t active);
void BCM_Input_ClearAll(void);
void BCM_Input_SetMode(BcmInput_Mode_t mode);
BcmInput_Mode_t BCM_Input_GetMode(void);
const char *BCM_Input_GetModeString(void);

#ifdef __cplusplus
}
#endif

#endif /* BCM_INPUT_H */
