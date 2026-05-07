#ifndef SIM_BRIDGE_H
#define SIM_BRIDGE_H

#include "main.h"
#include "cmsis_os2.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float    rpm;
    float    speed;
    bool     valid;
} SimBridgeData_t;

void SimBridge_Init(void);
void SimBridge_Task(void *argument);
void SimBridge_GetData(SimBridgeData_t *out);
bool SimBridge_IsConnected(void);

#endif