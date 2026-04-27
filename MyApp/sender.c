#include "gateway.h"
#include "cmsis_os.h"
#include "can.h"

extern CAN_HandleTypeDef hcan1;

uint16_t send_pot_value = 0;

void StartDefaultTask(void *argument)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[8];

    TxHeader.StdId = 0x102;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;

    // 필터 설정 (송신측도 필수)
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
    HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);

    HAL_CAN_Start(&hcan1);

    for(;;)
    {
        send_pot_value += 10;
        if(send_pot_value > 4095) send_pot_value = 0;

        TxData[0] = (uint8_t)(send_pot_value >> 8);
        TxData[1] = (uint8_t)(send_pot_value & 0xFF);

        if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) == HAL_OK) {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);  // 파란 LED — 송신 성공 확인용
        } else {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14); // 빨간 LED — 송신 실패
        }

        osDelay(500);
    }
}