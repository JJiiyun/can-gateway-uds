/* --- START OF FILE common/can_bsp.c --- */
#include "can_bsp.h"

extern CAN_HandleTypeDef hcan1;

// 게이트웨이 모드일 때만 CAN2 핸들을 가져옴
#ifdef BOARD_B_GATEWAY
extern CAN_HandleTypeDef hcan2;
#endif

osMessageQueueId_t can_rx_q = NULL;
#define CAN_RX_Q_SIZE 64 // 게이트웨이용 넉넉한 사이즈

HAL_StatusTypeDef CAN_BSP_Init(void)
{
    if (can_rx_q == NULL) {
        can_rx_q = osMessageQueueNew(CAN_RX_Q_SIZE, sizeof(CAN_RxMessage_t), NULL);
    }

    // ====== CAN1 초기화 ======
    CAN_FilterTypeDef filter1 = {0};
    filter1.FilterActivation = ENABLE;
    filter1.FilterBank = 0;
    filter1.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter1.FilterIdHigh = 0x0000; filter1.FilterIdLow = 0x0000;
    filter1.FilterMaskIdHigh = 0x0000; filter1.FilterMaskIdLow = 0x0000;
    filter1.FilterMode = CAN_FILTERMODE_IDMASK;
    filter1.FilterScale = CAN_FILTERSCALE_32BIT;
    filter1.SlaveStartFilterBank = 14; 

    HAL_CAN_ConfigFilter(&hcan1, &filter1);
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    // ====== CAN2 초기화 (Board B 전용) ======
#ifdef BOARD_B_GATEWAY
    CAN_FilterTypeDef filter2 = {0};
    filter2.FilterActivation = ENABLE;
    filter2.FilterBank = 14; // CAN2는 14번 뱅크부터 사용!
    filter2.FilterFIFOAssignment = CAN_FILTER_FIFO1; // CAN2는 FIFO1로 받음 (충돌 방지)
    filter2.FilterIdHigh = 0x0000; filter2.FilterIdLow = 0x0000;
    filter2.FilterMaskIdHigh = 0x0000; filter2.FilterMaskIdLow = 0x0000;
    filter2.FilterMode = CAN_FILTERMODE_IDMASK;
    filter2.FilterScale = CAN_FILTERSCALE_32BIT;
    filter2.SlaveStartFilterBank = 14; 

    HAL_CAN_ConfigFilter(&hcan2, &filter2);
    HAL_CAN_Start(&hcan2);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO1_MSG_PENDING);
#endif

    return HAL_OK;
}

// 특정 CAN 채널로 데이터 보내기 (게이트웨이 핵심 기능)
HAL_StatusTypeDef CAN_BSP_SendTo(CAN_HandleTypeDef *hcan, uint32_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef txHeader;
    uint32_t txMailbox;

    if (data == NULL || len > 8) return HAL_ERROR;
    if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0) return HAL_BUSY;

    txHeader.StdId = id;
    txHeader.DLC = len;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.TransmitGlobalTime = DISABLE;

    return HAL_CAN_AddTxMessage(hcan, &txHeader, data, &txMailbox);
}

// 기본 송신 (Board A 하위 호환용)
HAL_StatusTypeDef CAN_BSP_Send(uint32_t id, uint8_t *data, uint8_t len) {
    return CAN_BSP_SendTo(&hcan1, id, data, len);
}

bool CAN_BSP_Read(CAN_RxMessage_t *p_msg, uint32_t timeout) {
    if (can_rx_q != NULL && osMessageQueueGet(can_rx_q, p_msg, NULL, timeout) == osOK) {
        return true;
    }
    return false;
}

// ====== CAN1 수신 인터럽트 (Powertrain) ======
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    CAN_RxMessage_t rxMsg;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, rxMsg.data) == HAL_OK) {
        rxMsg.bus = 1; // CAN1에서 왔음 표시
        rxMsg.id = RxHeader.StdId;
        rxMsg.dlc = RxHeader.DLC;
        if (can_rx_q) osMessageQueuePut(can_rx_q, &rxMsg, 0, 0);
    }
}

#ifdef BOARD_B_GATEWAY
// ====== CAN2 수신 인터럽트 (Diagnostics) ======
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    CAN_RxMessage_t rxMsg;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, rxMsg.data) == HAL_OK) {
        rxMsg.bus = 2; // CAN2에서 왔음 표시
        rxMsg.id = RxHeader.StdId;
        rxMsg.dlc = RxHeader.DLC;
        if (can_rx_q) osMessageQueuePut(can_rx_q, &rxMsg, 0, 0);
    }
}
#endif