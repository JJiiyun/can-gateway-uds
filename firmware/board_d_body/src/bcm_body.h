/**
 * @file    bcm_body.h
 * @brief   Body Control Module simulation entry points.
 */

#ifndef BCM_BODY_H
#define BCM_BODY_H

#include <stdint.h>
#include "protocol_ids.h"

#ifdef __cplusplus
extern "C" {
#endif

void BCM_Body_Init(void);
void BCM_Body_InputTask(void *argument);
void BCM_Body_IgnRxTask(void *argument);
void BCM_Body_Task(void *argument);
void BCM_Body_OnCanRx(const CAN_Msg_t *msg);

uint8_t  BCM_Body_IsIgnOn(void);
uint32_t BCM_Body_GetTxCount(void);
uint32_t BCM_Body_GetRxCount(void);

/*
 * IGN override for bench/GUI operation.
 *   -1 = automatic, use CAN-derived IGN state
 *    0 = force IGN off / suppress turn transmission
 *    1 = force IGN on / allow turn transmission
 */
void BCM_Body_SetIgnOverride(int8_t override);
int8_t BCM_Body_GetIgnOverride(void);

#ifdef __cplusplus
}
#endif

#endif /* BCM_BODY_H */
