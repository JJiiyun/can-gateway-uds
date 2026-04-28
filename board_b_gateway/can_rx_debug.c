/* --- START OF FILE board_b_gateway/can_rx_debug.c --- */
#include "can_rx_debug.h"
#include "can_bsp.h"    // 공통 CAN 드라이버
#include "engine_can.h" // CAN ID 매크로
#include "cmsis_os2.h"

volatile uint32_t canRxCount = 0;
volatile uint16_t rx_rpm = 0;
volatile uint16_t rx_speed = 0;
volatile uint8_t rx_coolant = 0;
volatile uint8_t rx_throttle = 0;
volatile uint8_t rx_brake = 0;

// 🔥 보드 B (게이트웨이) : 수신 및 처리 테스크 오버라이딩
void StartDefaultTask(void *argument)
{
    // 공통 CAN 드라이버 초기화 (필터 설정, 큐 생성, 인터럽트 ON)
    CAN_BSP_Init();

    CAN_RxMessage_t rxMsg;

    for(;;)
    {
        // 큐에 데이터가 들어올 때까지 무한 대기 (CPU 0%)
        if (CAN_BSP_Read(&rxMsg, osWaitForever))
        {
            // 받은 편지가 엔진(0x100)에서 온 게 맞다면? 파싱!
            if (rxMsg.id == ENGINE_CAN_ID && rxMsg.dlc >= ENGINE_CAN_DLC)
            {
                canRxCount++;

                rx_rpm      = CAN_GetU16LE(rxMsg.data, 0);
                rx_speed    = CAN_GetU16LE(rxMsg.data, 2);
                rx_coolant  = rxMsg.data[4];
                rx_throttle = rxMsg.data[5];
                rx_brake    = rxMsg.data[6];

                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
            }
        }
    }
}

// 🔥 보드 B는 엔진 연산을 할 필요가 없으므로 해당 테스크 무력화!
void EngineSim_Task(void *argument)
{
    osThreadExit(); // 테스크 영구 종료
}