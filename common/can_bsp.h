/* --- START OF FILE common/can_bsp.h --- */
#ifndef CAN_BSP_H
#define CAN_BSP_H

#include "main.h"
#include "cmsis_os2.h"
#include "protocol_ids.h" // 프로토콜 ID 포함
#include <stdint.h>
#include <stdbool.h>


// 1. 게이트웨이를 위한 CAN 수신 데이터 구조체 정의
typedef struct {
    uint8_t  bus;       // 출처 확인용 (1: CAN1, 2: CAN2)
    uint32_t id;        // 수신된 CAN ID
    uint8_t  dlc;       // 데이터 길이
    uint8_t  data[8];   // 수신된 데이터
} CAN_RxMessage_t;

// 2. 외부에서 접근할 수 있는 RX 큐 핸들 (게이트웨이 Task 등에서 사용)
extern osMessageQueueId_t can_rx_q;

// 3. 통신 상태 카운터 (CLI 출력용)
extern volatile uint32_t can1TxCount;
extern volatile uint32_t can1RxCount;
extern volatile uint32_t canSendEnterCount;
extern volatile uint32_t canTxBusyCount;
extern volatile uint32_t canTxErrorCount;

// 4. BSP API 함수 선언
HAL_StatusTypeDef CAN_BSP_Init(void);
HAL_StatusTypeDef CAN_BSP_Send(uint32_t id, uint8_t *data, uint8_t len);

// 큐에서 데이터를 읽어오는 편의 함수
HAL_StatusTypeDef CAN_BSP_SendTo(CAN_HandleTypeDef *hcan, uint32_t id, uint8_t *data, uint8_t len);
bool CAN_BSP_Read(CAN_RxMessage_t *p_msg, uint32_t timeout);

#endif