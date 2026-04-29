// #include "rx_debug.h"

// extern CAN_HandleTypeDef hcan2;

// HAL_StatusTypeDef CAN2_BSP_Init(void)
// {
//     if (can_rx_q == NULL) {
//         can_rx_q = osMessageQueueNew(CAN_RX_Q_SIZE, sizeof(CAN_RxMessage_t), NULL);
//     }

//     CAN_FilterTypeDef filter2 = {0};
//     filter2.FilterActivation = ENABLE;
//     filter2.FilterBank = 14; // CAN2는 14번 뱅크부터 사용!
//     filter2.FilterFIFOAssignment = CAN_FILTER_FIFO0;
//     filter2.FilterIdHigh = 0x0000; filter2.FilterIdLow = 0x0000;
//     filter2.FilterMaskIdHigh = 0x0000; filter2.FilterMaskIdLow = 0x0000;
//     filter2.FilterMode = CAN_FILTERMODE_IDMASK;
//     filter2.FilterScale = CAN_FILTERSCALE_32BIT;
//     filter2.SlaveStartFilterBank = 14; 

//     HAL_CAN_ConfigFilter(&hcan2, &filter2);
//     HAL_CAN_Start(&hcan2);
//     HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);

//     return HAL_OK;
// }