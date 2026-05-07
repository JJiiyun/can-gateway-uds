#include "gateway_uds_server.h"

#include "can_cli_monitor.h"
#include "gateway_safety_bridge.h"
#include "protocol_ids.h"

#include <string.h>

extern CAN_HandleTypeDef hcan2;

#define ISOTP_SF_TYPE_MASK 0xF0U
#define ISOTP_SF_LEN_MASK  0x0FU

static void send_response(const uint8_t *payload, uint8_t payload_len)
{
    uint8_t data[8] = {0};
    HAL_StatusTypeDef status;

    if (payload == NULL || payload_len > 7U) {
        return;
    }

    data[0] = payload_len;
    memcpy(&data[1], payload, payload_len);

    status = CAN_BSP_SendTo(&hcan2, CAN_ID_GATEWAY_UDS_RESP, data, 8U);
    CanCliMonitor_LogTx(2U, CAN_ID_GATEWAY_UDS_RESP, data, 8U, status);
}

static void send_negative_response(uint8_t sid, uint8_t nrc)
{
    uint8_t payload[3];

    payload[0] = UDS_NEG_RESP;
    payload[1] = sid;
    payload[2] = nrc;
    send_response(payload, sizeof(payload));
}

static void handle_read_did(const uint8_t *request, uint8_t request_len)
{
    GatewaySafetyDiagnostic_t diag;
    uint16_t did;
    uint8_t payload[7] = {0};
    uint8_t payload_len = 0U;

    if (request_len != 3U) {
        send_negative_response(UDS_SID_READ_DID, UDS_NRC_INCORRECT_LENGTH);
        return;
    }

    did = ((uint16_t)request[1] << 8) | request[2];
    GatewaySafetyBridge_GetDiagnostic(&diag);

    payload[0] = UDS_POS_READ_DID;
    payload[1] = request[1];
    payload[2] = request[2];

    switch (did) {
    case UDS_DID_ADAS_STATUS:
        payload[3] = diag.flags;
        payload[4] = diag.risk_level;
        payload[5] = diag.front_distance_cm;
        payload[6] = diag.rear_distance_cm;
        payload_len = 7U;
        break;

    case UDS_DID_ADAS_FRONT_DISTANCE:
        payload[3] = diag.front_distance_cm;
        payload_len = 4U;
        break;

    case UDS_DID_ADAS_REAR_DISTANCE:
        payload[3] = diag.rear_distance_cm;
        payload_len = 4U;
        break;

    case UDS_DID_ADAS_FAULT_BITMAP:
        payload[3] = diag.active_fault_bitmap;
        payload[4] = diag.dtc_bitmap;
        payload_len = 5U;
        break;

    default:
        send_negative_response(UDS_SID_READ_DID, UDS_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    send_response(payload, payload_len);
}

static void handle_clear_dtc(const uint8_t *request, uint8_t request_len)
{
    uint8_t payload[1];

    if (request_len != 4U) {
        send_negative_response(UDS_SID_CLEAR_DTC, UDS_NRC_INCORRECT_LENGTH);
        return;
    }

    GatewaySafetyBridge_ClearDtc();
    payload[0] = UDS_POS_CLEAR_DTC;
    send_response(payload, sizeof(payload));
}

void GatewayUdsServer_OnRx(const CAN_RxMessage_t *rx_msg)
{
    uint8_t request_len;
    const uint8_t *request;
    uint8_t sid;

    if (rx_msg == NULL ||
        rx_msg->bus != 2U ||
        rx_msg->id != CAN_ID_GATEWAY_UDS_REQ ||
        rx_msg->dlc < 2U) {
        return;
    }

    if ((rx_msg->data[0] & ISOTP_SF_TYPE_MASK) != 0U) {
        return;
    }

    request_len = rx_msg->data[0] & ISOTP_SF_LEN_MASK;
    if (request_len == 0U || request_len > 7U || (uint8_t)(request_len + 1U) > rx_msg->dlc) {
        send_negative_response(rx_msg->data[1], UDS_NRC_INCORRECT_LENGTH);
        return;
    }

    request = &rx_msg->data[1];
    sid = request[0];

    switch (sid) {
    case UDS_SID_READ_DID:
        handle_read_did(request, request_len);
        break;

    case UDS_SID_CLEAR_DTC:
        handle_clear_dtc(request, request_len);
        break;

    default:
        send_negative_response(sid, UDS_NRC_SERVICE_NOT_SUPPORTED);
        break;
    }
}
