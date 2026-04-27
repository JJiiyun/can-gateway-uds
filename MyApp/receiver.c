/* Node B: receiver.c (수신측) */
#include "gateway.h"
#include "cmsis_os.h"
#include "can.h"

extern CAN_HandleTypeDef hcan1;

volatile uint16_t friend_pot_value = 0;
volatile uint32_t rx_cnt = 0;

void StartDefaultTask(void *argument)
{
    // 필터 설정
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
    // 필터 설정 실패 시 여기서 걸림
    Error_Handler();
}

    if(HAL_CAN_Start(&hcan1) != HAL_OK) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); 
    }
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    for(;;)
    {// CAN 상태를 체크해서 LED로 표시
        uint32_t error_code = HAL_CAN_GetError(&hcan1);
        if(error_code != HAL_CAN_ERROR_NONE) {
            // 에러가 발생하면 빨간 LED(PIN_0)가 깜빡이게 함
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        osDelay(500);
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
    {
        if (RxHeader.StdId == 0x102)
        {
            friend_pot_value = (RxData[0] << 8) | RxData[1];
            rx_cnt++;

            // 수신 성공 → 파란 LED 토글
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
        } else {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);   
        }
    } 
}