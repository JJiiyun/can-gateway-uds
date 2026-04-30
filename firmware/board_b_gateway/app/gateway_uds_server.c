#include "gateway_uds_server.h"

#include "can_cli_monitor.h"
#include "gateway_safety_bridge.h"
#include "protocol_ids.h"

#include <stdbool.h>
#include <string.h>

extern CAN_HandleTypeDef hcan2;

#define UDS_SID_READ_DATA_BY_IDENTIFIER      0x22U
#define UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION 0x14U
#define UDS_POSITIVE_RESPONSE_OFFSET         0x40U

#define UDS_NRC_INCORRECT_LENGTH             0x13U
#define UDS_NRC_REQUEST_OUT_OF_RANGE         0x31U
#define UDS_NRC_SERVICE_NOT_SUPPORTED        0x11U

static bool is_supported_request_id(uint32_t id)
{
    return id == CAN_ID_UDS_REQ_BOARD_B || id == CAN_ID_UDS_REQ_OBD_STD;
}

static uint32_t response_id_for_request(uint32_t request_id)
{
    return request_id == CAN_ID_UDS_REQ_OBD_STD ?
           CAN_ID_UDS_RESP_OBD_STD :
           CAN_ID_UDS_RESP_BOARD_B;
}

static void send_frame(uint32_t id, uint8_t data[8])
{
    HAL_StatusTypeDef status = CAN_BSP_SendTo(&hcan2, id, data, 8U);
    CanCliMonitor_LogTx(2U, id, data, 8U, status);
}

static void send_negative_response(uint32_t response_id,
                                   uint8_t original_sid,
                                   uint8_t nrc)
{
    uint8_t data[8] = {0};

    data[0] = 0x03U;
    data[1] = 0x7FU;
    data[2] = original_sid;
    data[3] = nrc;
    send_frame(response_id, data);
}

static void send_positive_did(uint32_t response_id,
                              uint16_t did,
                              const uint8_t *payload,
                              uint8_t payload_len)
{
    uint8_t data[8] = {0};

    if (payload_len > 4U) {
        payload_len = 4U;
    }

    data[0] = (uint8_t)(3U + payload_len);
    data[1] = UDS_SID_READ_DATA_BY_IDENTIFIER + UDS_POSITIVE_RESPONSE_OFFSET;
    data[2] = (uint8_t)(did >> 8);
    data[3] = (uint8_t)(did & 0xFFU);

    if (payload != NULL && payload_len > 0U) {
        memcpy(&data[4], payload, payload_len);
    }

    send_frame(response_id, data);
}

static void send_positive_clear_dtc(uint32_t response_id)
{
    uint8_t data[8] = {0};

    data[0] = 0x01U;
    data[1] = UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION + UDS_POSITIVE_RESPONSE_OFFSET;
    send_frame(response_id, data);
}

static void handle_read_data_by_identifier(const CAN_RxMessage_t *rx_msg,
                                           uint32_t response_id)
{
    GatewaySafetyDiagnostic_t diag;
    uint8_t payload[4] = {0};
    uint16_t did;

    if (rx_msg->dlc < 4U || rx_msg->data[0] < 3U) {
        send_negative_response(response_id,
                               UDS_SID_READ_DATA_BY_IDENTIFIER,
                               UDS_NRC_INCORRECT_LENGTH);
        return;
    }

    did = ((uint16_t)rx_msg->data[2] << 8) | rx_msg->data[3];
    GatewaySafetyBridge_GetDiagnostic(&diag);

    switch (did) {
    case UDS_DID_ADAS_STATUS:
        payload[0] = diag.flags;
        payload[1] = diag.risk_level;
        payload[2] = diag.front_distance_cm;
        payload[3] = diag.rear_distance_cm;
        send_positive_did(response_id, did, payload, 4U);
        break;

    case UDS_DID_ADAS_FRONT_DISTANCE:
        payload[0] = 0U;
        payload[1] = diag.front_distance_cm;
        send_positive_did(response_id, did, payload, 2U);
        break;

    case UDS_DID_ADAS_REAR_DISTANCE:
        payload[0] = 0U;
        payload[1] = diag.rear_distance_cm;
        send_positive_did(response_id, did, payload, 2U);
        break;

    case UDS_DID_ADAS_FAULT_BITMAP:
        payload[0] = diag.active_fault_bitmap;
        payload[1] = diag.dtc_bitmap;
        send_positive_did(response_id, did, payload, 2U);
        break;

    default:
        send_negative_response(response_id,
                               UDS_SID_READ_DATA_BY_IDENTIFIER,
                               UDS_NRC_REQUEST_OUT_OF_RANGE);
        break;
    }
}

static void handle_clear_diagnostic_information(const CAN_RxMessage_t *rx_msg,
                                                uint32_t response_id)
{
    if (rx_msg->dlc < 5U || rx_msg->data[0] < 4U) {
        send_negative_response(response_id,
                               UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION,
                               UDS_NRC_INCORRECT_LENGTH);
        return;
    }

    GatewaySafetyBridge_ClearDtc();
    send_positive_clear_dtc(response_id);
}

void GatewayUdsServer_OnRx(const CAN_RxMessage_t *rx_msg)
{
    uint32_t response_id;
    uint8_t sid;

    if (rx_msg == NULL ||
        rx_msg->bus != 2U ||
        !is_supported_request_id(rx_msg->id) ||
        rx_msg->dlc < 2U) {
        return;
    }

    response_id = response_id_for_request(rx_msg->id);
    sid = rx_msg->data[1];

    switch (sid) {
    case UDS_SID_READ_DATA_BY_IDENTIFIER:
        handle_read_data_by_identifier(rx_msg, response_id);
        break;

    case UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION:
        handle_clear_diagnostic_information(rx_msg, response_id);
        break;

    default:
        send_negative_response(response_id, sid, UDS_NRC_SERVICE_NOT_SUPPORTED);
        break;
    }
}
