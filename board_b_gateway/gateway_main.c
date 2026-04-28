/* --- START OF FILE board_b_gateway/gateway_main.c --- */
#include "can_bsp.h"
#include "uart.h"
#include "engine_can.h" // CAN_GetU16LE 등 사용
#include <stdio.h>

// CubeMX에서 생성한 CAN2 핸들
extern CAN_HandleTypeDef hcan2; 

void StartDefaultTask(void *argument)
{
    uartInit();
    CAN_BSP_Init(); // CAN1, CAN2 모두 활성화됨

    printf("\r\n[Gateway] Central Gateway Started.\r\n");
    
    CAN_RxMessage_t rxMsg;

    for(;;)
    {
        // 양쪽 버스(CAN1, CAN2)에서 오는 모든 메시지를 하나의 큐에서 꺼냄
        if (CAN_BSP_Read(&rxMsg, osWaitForever))
        {
            // ==========================================
            // [라우팅 규칙 1] CAN1(엔진) -> CAN2(계기판/UDS)
            // ==========================================
            if (rxMsg.bus == 1) 
            {
                if (rxMsg.id == CAN_ID_ENGINE_DATA) // 0x100
                {
                    // 1. 이상 감지 로직 (RPM이 5000 이상이면 경고!)
                    uint16_t rpm = CAN_GetU16LE(rxMsg.data, 0);
                    if (rpm >= 5000) {
                        uint8_t warning_data[8] = {0xFF, 0x01, 0, 0, 0, 0, 0, 0};
                        CAN_BSP_SendTo(&hcan2, CAN_ID_DASHBOARD_CTRL, warning_data, 8);
                        printf("[WARN] High RPM: %d! Warning Sent.\r\n", rpm);
                    }

                    // 2. 포워딩 (CAN1에서 받은 엔진 데이터를 그대로 CAN2로 뿌림)
                    // 계기판(실차)이나 Board C가 이 데이터를 보고 바늘을 움직입니다.
                    CAN_BSP_SendTo(&hcan2, CAN_ID_ENGINE_DATA, rxMsg.data, rxMsg.dlc);
                    
                    // 3. 통계/로거 출력
                    printf("[Route] Engine(CAN1) -> CAN2 | RPM: %d\r\n", rpm);
                }
            }
            // ==========================================
            // [라우팅 규칙 2] CAN2(UDS 진단기) -> CAN1(엔진) 
            // ==========================================
            else if (rxMsg.bus == 2) 
            {
                // 진단기(Board C)가 엔진 데이터를 요청(0x7E0)하면 엔진(CAN1)으로 전달
                if (rxMsg.id == CAN_ID_UDS_REQ_BOARD_B) 
                {
                    printf("[Route] UDS Req(CAN2) -> CAN1\r\n");
                    // CAN_BSP_SendTo(&hcan1, rxMsg.id, rxMsg.data, rxMsg.dlc);
                }
            }
        }  
    }
}

void EngineSim_Task(void *argument) {
    osThreadExit();
}