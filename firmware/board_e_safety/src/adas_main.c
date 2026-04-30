#include "adas_main.h"

#include "adas_can.h"
#include "adas_input.h"
#include "adas_signal.h"
#include "cmsis_os.h"
#include "uart.h"

#include <string.h>

#define ADAS_INPUT_PERIOD_MS          20U
#define ADAS_STATUS_PERIOD_MS         100U
#define ADAS_LOG_PERIOD_MS            1000U

#define ADAS_FRONT_WARN_CM            50U
#define ADAS_FRONT_DANGER_CM          30U
#define ADAS_REAR_WARN_CM             25U
#define ADAS_FRONT_WARN_SPEED_KMH     20U
#define ADAS_FRONT_DANGER_SPEED_KMH   40U
#define ADAS_HARSH_BRAKE_SPEED_KMH    30U

static AdasStatus_t s_status;
static uint8_t s_initialized;

static uint8_t max_u8(uint8_t a, uint8_t b)
{
    return a > b ? a : b;
}

static uint8_t clamp_speed(uint16_t speed_kmh)
{
    return speed_kmh > 255U ? 255U : (uint8_t)speed_kmh;
}

static void evaluate_status(uint8_t alive)
{
    AdasInputState_t input;
    uint16_t speed_kmh = ADAS_Can_GetVehicleSpeedKmh();
    AdasStatus_t next = {0};

    ADAS_Input_GetState(&input);

    next.flags = ADAS_FLAG_ACTIVE;
    next.front_distance_cm = input.front_distance_cm;
    next.rear_distance_cm = input.rear_distance_cm;
    next.speed_kmh = clamp_speed(speed_kmh);
    next.alive = alive;

    if (input.lane_departure) {
        next.flags |= ADAS_FLAG_LANE_DEPARTURE;
        next.input_bitmap |= ADAS_INPUT_LANE_BUTTON;
        next.risk_level = max_u8(next.risk_level, 2U);
    }

    if (input.harsh_brake) {
        next.flags |= ADAS_FLAG_HARSH_BRAKE;
        next.input_bitmap |= ADAS_INPUT_BRAKE_BUTTON;
        if (speed_kmh >= ADAS_HARSH_BRAKE_SPEED_KMH) {
            next.risk_level = max_u8(next.risk_level, 2U);
        } else {
            next.risk_level = max_u8(next.risk_level, 1U);
        }
    }

    if (input.front_distance_cm <= ADAS_FRONT_DANGER_CM &&
        speed_kmh >= ADAS_FRONT_DANGER_SPEED_KMH) {
        next.flags |= ADAS_FLAG_FRONT_COLLISION;
        next.risk_level = max_u8(next.risk_level, 3U);
    } else if (input.front_distance_cm <= ADAS_FRONT_WARN_CM &&
               speed_kmh >= ADAS_FRONT_WARN_SPEED_KMH) {
        next.flags |= ADAS_FLAG_FRONT_COLLISION;
        next.risk_level = max_u8(next.risk_level, 2U);
    }

    if (input.rear_distance_cm <= ADAS_REAR_WARN_CM) {
        next.flags |= ADAS_FLAG_REAR_OBSTACLE;
        next.risk_level = max_u8(next.risk_level, 1U);
    }

    if (input.sensor_fault) {
        next.flags |= ADAS_FLAG_SENSOR_FAULT;
        next.input_bitmap |= ADAS_INPUT_SENSOR_FAULT;
        next.fault_bitmap |= ADAS_FAULT_FRONT_SENSOR | ADAS_FAULT_REAR_SENSOR;
        next.risk_level = max_u8(next.risk_level, 2U);
    }

    s_status = next;
}

static uint8_t status_changed_for_log(const AdasStatus_t *a, const AdasStatus_t *b)
{
    return a->flags != b->flags ||
           a->risk_level != b->risk_level ||
           a->front_distance_cm != b->front_distance_cm ||
           a->rear_distance_cm != b->rear_distance_cm ||
           a->fault_bitmap != b->fault_bitmap ||
           a->speed_kmh != b->speed_kmh;
}

static void log_status_if_needed(uint8_t force)
{
    static uint8_t valid;
    static AdasStatus_t last;

    if (force || !valid || status_changed_for_log(&s_status, &last)) {
        uartPrintf(0,
                   "[ADAS] risk=%u flags=0x%02X front=%ucm rear=%ucm speed=%ukm/h fault=0x%02X\r\n",
                   s_status.risk_level,
                   s_status.flags,
                   s_status.front_distance_cm,
                   s_status.rear_distance_cm,
                   s_status.speed_kmh,
                   s_status.fault_bitmap);
        last = s_status;
        valid = 1U;
    }
}

static void ADAS_Safety_Init(void)
{
    if (s_initialized) {
        return;
    }

    memset(&s_status, 0, sizeof(s_status));
    (void)uartInit();
    ADAS_Input_Init();
    (void)ADAS_Can_Init();
    s_initialized = 1U;

    uartPrintf(0, "[ADAS] Board E Safety ECU ready\r\n");
}

void ADAS_Safety_Task(void *argument)
{
    uint32_t now;
    uint32_t next_input_tick;
    uint32_t next_status_tick;
    uint32_t next_log_tick;
    uint8_t alive = 0U;

    (void)argument;
    ADAS_Safety_Init();

    now = osKernelGetTickCount();
    next_input_tick = now;
    next_status_tick = now;
    next_log_tick = now + ADAS_LOG_PERIOD_MS;

    for (;;) {
        ADAS_Can_PollRx(0U);
        now = osKernelGetTickCount();

        if ((uint32_t)(now - next_input_tick) < 0x80000000UL) {
            ADAS_Input_Poll();
            next_input_tick = now + ADAS_INPUT_PERIOD_MS;
        }

        if ((uint32_t)(now - next_status_tick) < 0x80000000UL) {
            evaluate_status(alive++);
            (void)ADAS_Can_SendStatus(&s_status);
            log_status_if_needed(0U);
            next_status_tick = now + ADAS_STATUS_PERIOD_MS;
        }

        if ((uint32_t)(now - next_log_tick) < 0x80000000UL) {
            AdasCanStats_t stats;
            ADAS_Can_GetStats(&stats);
            uartPrintf(0, "[ADAS] tx=%lu rx=%lu tx_err=%lu engine=%u\r\n",
                       (unsigned long)stats.tx_count,
                       (unsigned long)stats.rx_count,
                       (unsigned long)stats.tx_error_count,
                       (unsigned int)stats.engine_active);
            log_status_if_needed(1U);
            next_log_tick = now + ADAS_LOG_PERIOD_MS;
        }

        osDelay(5U);
    }
}

void EngineSim_Task(void *argument)
{
    ADAS_Safety_Task(argument);
}
