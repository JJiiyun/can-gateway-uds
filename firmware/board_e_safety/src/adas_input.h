#ifndef ADAS_INPUT_H
#define ADAS_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t front_distance_cm;
    uint8_t rear_distance_cm;
    uint8_t sensor_fault;
    uint16_t front_echo_us;
    uint16_t rear_echo_us;
} AdasInputState_t;

void ADAS_Input_Init(void);
void ADAS_Input_Poll(void);
void ADAS_Input_GetState(AdasInputState_t *out_state);

#ifdef __cplusplus
}
#endif

#endif /* ADAS_INPUT_H */
