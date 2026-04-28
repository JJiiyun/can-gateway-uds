#include "main.h"
#include "cmsis_os2.h"
#include "can_bsp.h"      // common: CAN 수신용
#include "uart.h"         // common: uartReadBlock 사용
#include "uds_server.h"    // 친구 코드: UDS 데이터 갱신용
#include "cli_cmd.h"       // 친구 코드: UDS_CLI_ProcessChar 사용
#include "protocol_ids.h"  // common: CAN ID 정의

/**
 * @brief MX에서 생성한 __weak 함수를 오버라이딩
 * freertos.c를 건드리지 않고 이 함수가 실질적인 Board C의 메인이 됩니다.
 */
void UDSMainTask(void *argument) {
    /* 1. 초기화 (Common BSP + 친구의 Server/CLI 초기화) */
    uartInit();         // UART3 및 수신 큐 활성화
    CAN_BSP_Init();     // CAN 필터 및 인터럽트 활성화
    UDS_Server_Init();  // 친구: 서버 데이터 초기화 (RPM 1234 등)
    UDS_CLI_Init();     // 친구: CLI 버퍼 초기화

    // 친구가 원한 시작 메시지 출력 (uartPrintf 사용)
    uartPrintf(0, "\r\n[Board C] UDS Tester Online\r\n> ");

    uint8_t rx_char;
    CAN_RxMessage_t canRxMsg;

    for(;;) {
        /* [기능 1] UART 입력 처리 (친구의 CLI 로직 100% 활용) */
        // common의 큐에서 한 글자씩 꺼내 친구의 ProcessChar로 넘깁니다.
        if (uartReadBlock(0, &rx_char, 1) == true) {
            UDS_CLI_ProcessChar((char)rx_char);
        }

        /* [기능 2] CAN 수신 데이터 동기화 (Gateway 데이터 반영) */
        // Gateway(Board B)에서 온 센서 데이터를 UDS 서버 변수에 실시간 업데이트
        if (CAN_BSP_Read(&canRxMsg, 0)) {
            if (canRxMsg.id == CAN_ID_ENGINE_DATA) {
                uint16_t rpm = (canRxMsg.data[0] << 8) | canRxMsg.data[1];
                uint16_t speed = (canRxMsg.data[2] << 8) | canRxMsg.data[3];
                int8_t temp = (int8_t)canRxMsg.data[4];

                UDS_Update_RPM(rpm);
                UDS_Update_Speed(speed);
                UDS_Update_Temp(temp);
            }
        }

        osDelay(1); // Task 스케줄링을 위한 최소 딜레이
    }
}