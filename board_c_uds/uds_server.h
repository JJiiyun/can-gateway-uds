#ifndef UDS_SERVER_H
#define UDS_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#define UDS_SID_READ_DID              0x22
#define UDS_POS_READ_DID              0x62

#define UDS_NRC_SERVICE_NOT_SUPPORTED 0x11
#define UDS_NRC_INCORRECT_LENGTH      0x13
#define UDS_NRC_REQUEST_OUT_OF_RANGE  0x31

#define UDS_DID_VIN                   0xF190
#define UDS_DID_RPM                   0xF40C
#define UDS_DID_SPEED                 0xF40D
#define UDS_DID_TEMP                  0xF40E

#define UDS_RESP_MAX_LEN              32

typedef struct
{
    uint8_t data[UDS_RESP_MAX_LEN];
    uint8_t len;
} uds_resp_t;

void UDS_Server_Init(void);

void UDS_Update_RPM(uint16_t rpm);
void UDS_Update_Speed(uint16_t speed);
void UDS_Update_Temp(int8_t temp);

bool UDS_Server_Process(const uint8_t *req, uint8_t req_len, uds_resp_t *resp);

#endif