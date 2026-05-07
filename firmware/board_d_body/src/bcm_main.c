/**
 * @file    bcm_main.c
 * @brief   BCM FreeRTOS task orchestration.
 */

#include "bcm_body.h"
#include "bcm_can.h"
#include "bcm_cli.h"
#include "bcm_input.h"
#include "bcm_signal.h"
#include "cmsis_os.h"
#include "cli.h"
#include "uart.h"

/*
 * Stage switch:
 *   0 = standalone bench mode, send turn frame every 100 ms immediately.
 *   1 = integration mode, send turn frame only after an IGN-on CAN frame is received.
 */
#ifndef BCM_BODY_WAIT_FOR_IGN
#define BCM_BODY_WAIT_FOR_IGN 1
#endif

#ifndef PERIOD_TURN_BLINK_MS
#define PERIOD_TURN_BLINK_MS 500U
#endif

#ifndef PERIOD_BODY_STATUS_MS
#define PERIOD_BODY_STATUS_MS 100U
#endif

#ifndef PERIOD_INPUT_DIAG_LOG_MS
#define PERIOD_INPUT_DIAG_LOG_MS 250U
#endif

#if defined(__GNUC__)
#define BCM_NOINLINE __attribute__((noinline))
#else
#define BCM_NOINLINE
#endif

static volatile uint8_t s_initialized;
static osThreadId_t s_input_task_handle;
static osThreadId_t s_ign_rx_task_handle;
static uint8_t s_input_log_valid;
static BcmInput_State_t s_last_logged_input;
static BcmInput_State_t s_last_logged_raw;
static uint32_t s_last_input_diag_tick;
static volatile int8_t s_ign_override = -1;

static const osThreadAttr_t s_input_task_attributes = {
    .name = "bcmInput",
    .stack_size = 512U * 4U,
    .priority = (osPriority_t)osPriorityLow,
};

static const osThreadAttr_t s_ign_rx_task_attributes = {
    .name = "bcmIgnRx",
    .stack_size = 512U * 4U,
    .priority = (osPriority_t)osPriorityLow,
};

static uint8_t should_transmit(void)
{
#if BCM_BODY_WAIT_FOR_IGN
    if (s_ign_override >= 0) {
        return s_ign_override ? 1U : 0U;
    }

    return BCM_Can_IsIgnOn();
#else
    return 1U;
#endif
}

static uint8_t input_changed(const BcmInput_State_t *a, const BcmInput_State_t *b)
{
    return a->turn_left_enabled != b->turn_left_enabled ||
           a->turn_right_enabled != b->turn_right_enabled ||
           a->hazard_enabled != b->hazard_enabled;
}

static void log_input_if_changed(const BcmInput_State_t *input)
{
    BcmInput_State_t raw;
    uint32_t now = HAL_GetTick();
    uint8_t due = ((uint32_t)(now - s_last_input_diag_tick) >= PERIOD_INPUT_DIAG_LOG_MS) ? 1U : 0U;

    BCM_Input_GetRawState(&raw);

    if (!s_input_log_valid ||
        input_changed(input, &s_last_logged_input) ||
        input_changed(&raw, &s_last_logged_raw) ||
        due) {
        uartPrintf(0,
                   "[BCM] INPUT raw=L%u/R%u/H%u turn=L%u/R%u/H%u\r\n",
                   raw.turn_left_enabled,
                   raw.turn_right_enabled,
                   raw.hazard_enabled,
                   input->turn_left_enabled,
                   input->turn_right_enabled,
                   input->hazard_enabled);
        s_last_logged_raw = raw;
        s_last_logged_input = *input;
        s_input_log_valid = 1U;
        s_last_input_diag_tick = now;
    }
}

void BCM_Body_Init(void)
{
    if (s_initialized) {
        return;
    }

    BCM_Input_Init();
    if (!uartInit()) {
        return;
    }

    cliInit();
    BCM_Cli_Init();

    (void)BCM_Can_Init();
    (void)BCM_Body_IsIgnOn();
    (void)BCM_Body_GetTxCount();
    (void)BCM_Body_GetRxCount();
    s_initialized = 1U;

    uartPrintf(0, "\033[2J\033[H");
    uartPrintf(0, "==================================\r\n");
    uartPrintf(0, "   Turn Signal Simulator\r\n");
    uartPrintf(0, "   Type 'help' or 'bcm_help'\r\n");
    uartPrintf(0, "==================================\r\n\r\n");
    uartPrintf(0, "[BCM] Turn module ready, wait_for_ign=%u\r\n",
               (unsigned)BCM_BODY_WAIT_FOR_IGN);
    uartPrintf(0, "CLI > ");
}

void BCM_Body_InputTask(void *argument)
{
    (void)argument;

    for (;;) {
        BCM_Input_Poll();
        osDelay(20U);
    }
}

void BCM_Body_IgnRxTask(void *argument)
{
    (void)argument;

    for (;;) {
        BCM_Can_PollRx(100U);
    }
}

void BCM_Body_Task(void *argument)
{
    (void)argument;

    BCM_Body_Init();

    uint32_t last_status_tick = HAL_GetTick();

    for (;;) {
        uint32_t now = HAL_GetTick();

        cliMain();
        BCM_Cli_Process();

        if ((now - last_status_tick) >= PERIOD_BODY_STATUS_MS) {
            BcmSignal_TurnStatus_t status;
            CAN_Msg_t msg;
            uint8_t blink_on = ((now / PERIOD_TURN_BLINK_MS) % 2U) == 0U ? 1U : 0U;

            last_status_tick = now;

            BCM_Input_GetState(&status.input);
            log_input_if_changed(&status.input);
            status.left_blink_on = blink_on;
            status.right_blink_on = blink_on;
            BCM_Signal_BuildTurnFrame(&status, &msg);

            if (should_transmit()) {
                (void)BCM_Can_SendTurnStatus(&msg);
            }
        }

        osDelay(1U);
    }
}

void BCM_Body_OnCanRx(const CAN_Msg_t *msg)
{
    BCM_Can_OnRx(msg);
}

BCM_NOINLINE uint8_t BCM_Body_IsIgnOn(void)
{
    return should_transmit();
}

BCM_NOINLINE uint32_t BCM_Body_GetTxCount(void)
{
    BcmCan_Stats_t stats;

    BCM_Can_GetStats(&stats);
    return stats.tx_count;
}

BCM_NOINLINE uint32_t BCM_Body_GetRxCount(void)
{
    BcmCan_Stats_t stats;

    BCM_Can_GetStats(&stats);
    return stats.rx_count;
}

void BCM_Body_SetIgnOverride(int8_t override)
{
    s_ign_override = override < 0 ? -1 : (override ? 1 : 0);
}

int8_t BCM_Body_GetIgnOverride(void)
{
    return s_ign_override;
}

/*
 * The current CubeMX FreeRTOS file creates EngineSim_Task unconditionally.
 * For the BCM build role this strong symbol reuses that slot as BodyTxTask,
 * then starts the separate InputTask.
 */
void EngineSim_Task(void *argument)
{
    BCM_Body_Init();

    if (s_input_task_handle == NULL) {
        s_input_task_handle = osThreadNew(BCM_Body_InputTask, NULL, &s_input_task_attributes);
    }
    if (s_ign_rx_task_handle == NULL) {
        s_ign_rx_task_handle = osThreadNew(BCM_Body_IgnRxTask, NULL, &s_ign_rx_task_attributes);
    }

    BCM_Body_Task(argument);
}
