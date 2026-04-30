#ifndef GATEWAY_UDS_SERVER_H
#define GATEWAY_UDS_SERVER_H

#include "can_bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

void GatewayUdsServer_OnRx(const CAN_RxMessage_t *rx_msg);

#ifdef __cplusplus
}
#endif

#endif /* GATEWAY_UDS_SERVER_H */
