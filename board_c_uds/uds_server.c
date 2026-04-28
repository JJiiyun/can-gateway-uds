#include "uds_server.h"
#include <string.h>

static uint16_t g_rpm = 1234;
static uint16_t g_speed = 56;
static int8_t   g_temp = 85;
static const uint8_t g_vin[4] = { 'K', 'M', 'H', '1' };  // 과제용 partial VIN

static void uds_make_negative_response(uint8_t sid, uint8_t nrc, uds_resp_t *resp)
{
    resp->data[0] = 0x7F;
    resp->data[1] = sid;
    resp->data[2] = nrc;
    resp->len = 3;
}

static bool uds_handle_read_did(const uint8_t *req, uint8_t req_len, uds_resp_t *resp)
{
    uint16_t did;

    if (req_len != 3)
    {
        uds_make_negative_response(UDS_SID_READ_DID, UDS_NRC_INCORRECT_LENGTH, resp);
        return true;
    }

    did = ((uint16_t)req[1] << 8) | req[2];

    resp->data[0] = UDS_POS_READ_DID;
    resp->data[1] = req[1];
    resp->data[2] = req[2];

    switch (did)
    {
        case UDS_DID_VIN:
            resp->data[3] = g_vin[0];
            resp->data[4] = g_vin[1];
            resp->data[5] = g_vin[2];
            resp->data[6] = g_vin[3];
            resp->len = 7;
            return true;

        case UDS_DID_RPM:
            resp->data[3] = (uint8_t)((g_rpm >> 8) & 0xFF);
            resp->data[4] = (uint8_t)(g_rpm & 0xFF);
            resp->len = 5;
            return true;

        case UDS_DID_SPEED:
            resp->data[3] = (uint8_t)((g_speed >> 8) & 0xFF);
            resp->data[4] = (uint8_t)(g_speed & 0xFF);
            resp->len = 5;
            return true;

        case UDS_DID_TEMP:
            resp->data[3] = (uint8_t)g_temp;
            resp->len = 4;
            return true;

        default:
            uds_make_negative_response(UDS_SID_READ_DID, UDS_NRC_REQUEST_OUT_OF_RANGE, resp);
            return true;
    }
}

void UDS_Server_Init(void)
{
    g_rpm = 1234;
    g_speed = 56;
    g_temp = 85;
}

void UDS_Update_RPM(uint16_t rpm)
{
    g_rpm = rpm;
}

void UDS_Update_Speed(uint16_t speed)
{
    g_speed = speed;
}

void UDS_Update_Temp(int8_t temp)
{
    g_temp = temp;
}

bool UDS_Server_Process(const uint8_t *req, uint8_t req_len, uds_resp_t *resp)
{
    if (req == NULL || resp == NULL || req_len == 0)
    {
        return false;
    }

    memset(resp, 0, sizeof(uds_resp_t));

    switch (req[0])
    {
        case UDS_SID_READ_DID:
            return uds_handle_read_did(req, req_len, resp);

        default:
            uds_make_negative_response(req[0], UDS_NRC_SERVICE_NOT_SUPPORTED, resp);
            return true;
    }
}