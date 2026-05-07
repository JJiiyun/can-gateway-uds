#include "ap.h"

void StartDefaultTask(void *argument)
{
    // 1. 하드웨어/통신 드라이버 초기화 (Queue, Mutex 등 RTOS 객체 생성 포함)
    uartInit();

    // 2. 미들웨어/인터페이스 초기화
    cliInit();
    CliCmd_Init(); // 프로젝트 전용 CLI 명령어 등록

    // 3. 비즈니스 로직 초기화
    EngineSim_Init();

    CAN_BSP_Init();

    // 화면 클리어 및 시작 메시지 출력
    uartPrintf(0, "\033[2J\033[H");
    uartPrintf(0, "==================================\r\n");
    uartPrintf(0, "   Engine ECU Simulator\r\n");
    uartPrintf(0, "   Build: %s %s\r\n", __DATE__, __TIME__);
    uartPrintf(0, "   Type 'help'\r\n");
    uartPrintf(0, "==================================\r\n\r\n");
    uartPrintf(0, "CLI > ");

    while (1)
    {
        // CLI 입력 처리 (블로킹 되지 않도록 설계되어야 함)
        //시뮬레이션 사용 시 cilMain() 주석 처리
        //cliMain();
        CliCmd_Process();

        // 다른 Task(EngSimTask 등)에게 CPU 점유율을 양보
        osDelay(1);
    }

}

void StartEngSimTask(void *argument)
{
	EngineSim_Task(argument); 
}
