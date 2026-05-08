#include "adas_main.h"

#include "adas_can.h"
#include "adas_input.h"
#include "adas_signal.h"
#include "cli.h"
#include "cmsis_os.h"
#include "uart.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef ADAS_INPUT_PERIOD_MS
#define ADAS_INPUT_PERIOD_MS          60U
#endif

#ifndef ADAS_STATUS_PERIOD_MS
#define ADAS_STATUS_PERIOD_MS         100U
#endif

#ifndef ADAS_LOG_PERIOD_MS
#define ADAS_LOG_PERIOD_MS            1000U
#endif

#define ADAS_MONITOR_DEFAULT_INTERVAL_MS 500U
#define ADAS_MONITOR_MIN_INTERVAL_MS     100U

#define ADAS_FRONT_WARN_CM            50U
#define ADAS_FRONT_DANGER_CM          30U
#define ADAS_REAR_WARN_CM             25U
#define ADAS_FRONT_WARN_SPEED_KMH     20U
#define ADAS_FRONT_DANGER_SPEED_KMH   40U

static AdasStatus_t s_status;
static uint8_t s_initialized;
static bool s_periodic_log_enabled = false;
static bool s_event_log_enabled = false;
static bool s_monitor_enabled = false;
static uint32_t s_monitor_interval_ms = ADAS_MONITOR_DEFAULT_INTERVAL_MS;
static uint32_t s_monitor_last_tick;
static osThreadId_t s_input_task_handle;

static const osThreadAttr_t s_input_task_attributes = {
    .name = "adasInput",
    .stack_size = 512U * 4U,
    .priority = (osPriority_t)osPriorityLow,
};

static void ADAS_InputTask(void *argument);

static uint32_t parse_number(const char *str)
{
    if (str == NULL) {
        return 0U;
    }
    return (uint32_t)strtoul(str, NULL, 0);
}

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
        next.fault_bitmap |= ADAS_FAULT_FRONT_SENSOR | ADAS_FAULT_REAR_SENSOR;
        next.input_bitmap |= ADAS_INPUT_SENSOR_FAULT;
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
           a->input_bitmap != b->input_bitmap ||
           a->speed_kmh != b->speed_kmh;
}

static void log_status_if_needed(uint8_t force)
{
    static uint8_t valid;
    static AdasStatus_t last;

    if (!force && !s_event_log_enabled) {
        return;
    }

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

static void print_status_cli(void)
{
    AdasCanStats_t stats;
    AdasInputState_t input;

    ADAS_Can_GetStats(&stats);
    ADAS_Input_GetState(&input);

    cliPrintf("\r\n[ADAS Status]\r\n");
    cliPrintf("risk       : %u\r\n", (unsigned int)s_status.risk_level);
    cliPrintf("flags      : 0x%02X\r\n", (unsigned int)s_status.flags);
    cliPrintf("front/rear : %u cm / %u cm\r\n",
              (unsigned int)s_status.front_distance_cm,
              (unsigned int)s_status.rear_distance_cm);
    cliPrintf("echo       : %u us / %u us\r\n",
              (unsigned int)input.front_echo_us,
              (unsigned int)input.rear_echo_us);
    cliPrintf("speed      : %u km/h, engine=%u\r\n",
              (unsigned int)stats.speed_kmh,
              (unsigned int)stats.engine_active);
    cliPrintf("fault      : 0x%02X, input=0x%02X, alive=%u\r\n",
              (unsigned int)s_status.fault_bitmap,
              (unsigned int)s_status.input_bitmap,
              (unsigned int)s_status.alive);
    cliPrintf("can        : tx=%lu rx=%lu err=%lu\r\n",
              (unsigned long)stats.tx_count,
              (unsigned long)stats.rx_count,
              (unsigned long)stats.tx_error_count);
    cliPrintf("3A0        : dt=%lu ms, age=%lu ms\r\n",
              (unsigned long)stats.status_tx_period_ms,
              (unsigned long)stats.status_tx_age_ms);
    cliPrintf("rx         : dt=%lu ms, age=%lu ms\r\n",
              (unsigned long)stats.rx_period_ms,
              (unsigned long)stats.rx_age_ms);
    cliPrintf("ign/spd age: %lu ms / %lu ms\r\n",
              (unsigned long)stats.ign_rx_age_ms,
              (unsigned long)stats.speed_rx_age_ms);
    cliPrintf("log        : periodic=%s, event=%s\r\n",
              s_periodic_log_enabled ? "on" : "off",
              s_event_log_enabled ? "on" : "off");
    cliPrintf("monitor    : %s, interval=%lu ms\r\n\r\n",
              s_monitor_enabled ? "on" : "off",
              (unsigned long)s_monitor_interval_ms);
}

static void print_monitor_line(void)
{
    AdasCanStats_t stats;

    ADAS_Can_GetStats(&stats);
    cliPrintf("ADAS risk=%u flags=0x%02X front=%u rear=%u speed=%u tx=%lu rx=%lu\r\n",
              (unsigned int)s_status.risk_level,
              (unsigned int)s_status.flags,
              (unsigned int)s_status.front_distance_cm,
              (unsigned int)s_status.rear_distance_cm,
              (unsigned int)s_status.speed_kmh,
              (unsigned long)stats.tx_count,
              (unsigned long)stats.rx_count);
    cliPrintf("ADAS 3A0 dt=%lums age=%lums ign_age=%lums spd_age=%lums\r\n",
              (unsigned long)stats.status_tx_period_ms,
              (unsigned long)stats.status_tx_age_ms,
              (unsigned long)stats.ign_rx_age_ms,
              (unsigned long)stats.speed_rx_age_ms);
}

static void print_usage(void)
{
    cliPrintf("--------ADAS Commands-------------\r\n");
    cliPrintf("adas status\r\n");
    cliPrintf("adas monitor on [interval_ms]\r\n");
    cliPrintf("adas monitor off\r\n");
    cliPrintf("adas monitor once\r\n");
    cliPrintf("adas log on|off|stat\r\n");
    cliPrintf("adas event on|off|stat\r\n");
    cliPrintf("----------------------------------\r\n");
}

static void handle_monitor(uint8_t argc, char *argv[])
{
    if (argc < 3U) {
        cliPrintf("usage: adas monitor on [interval_ms] | off | once\r\n");
        return;
    }

    if (strcmp(argv[2], "on") == 0) {
        if (argc >= 4U) {
            uint32_t interval = parse_number(argv[3]);

            if (interval < ADAS_MONITOR_MIN_INTERVAL_MS) {
                interval = ADAS_MONITOR_MIN_INTERVAL_MS;
            }
            s_monitor_interval_ms = interval;
        }
        s_monitor_last_tick = osKernelGetTickCount();
        s_monitor_enabled = true;
        cliPrintf("adas monitor on, interval=%lu ms\r\n",
                  (unsigned long)s_monitor_interval_ms);
        return;
    }

    if (strcmp(argv[2], "off") == 0) {
        s_monitor_enabled = false;
        cliPrintf("adas monitor off\r\n");
        return;
    }

    if (strcmp(argv[2], "once") == 0) {
        print_monitor_line();
        return;
    }

    cliPrintf("usage: adas monitor on [interval_ms] | off | once\r\n");
}

static void handle_log(uint8_t argc, char *argv[])
{
    if (argc < 3U || strcmp(argv[2], "stat") == 0) {
        cliPrintf("adas log periodic=%s\r\n",
                  s_periodic_log_enabled ? "on" : "off");
        return;
    }

    if (strcmp(argv[2], "on") == 0) {
        s_periodic_log_enabled = true;
        cliPrintf("adas log on\r\n");
        return;
    }

    if (strcmp(argv[2], "off") == 0) {
        s_periodic_log_enabled = false;
        cliPrintf("adas log off\r\n");
        return;
    }

    cliPrintf("usage: adas log on | off | stat\r\n");
}

static void handle_event(uint8_t argc, char *argv[])
{
    if (argc < 3U || strcmp(argv[2], "stat") == 0) {
        cliPrintf("adas event log=%s\r\n",
                  s_event_log_enabled ? "on" : "off");
        return;
    }

    if (strcmp(argv[2], "on") == 0) {
        s_event_log_enabled = true;
        cliPrintf("adas event on\r\n");
        return;
    }

    if (strcmp(argv[2], "off") == 0) {
        s_event_log_enabled = false;
        cliPrintf("adas event off\r\n");
        return;
    }

    cliPrintf("usage: adas event on | off | stat\r\n");
}

static void cmd_adas(uint8_t argc, char *argv[])
{
    if (argc < 2U || strcmp(argv[1], "help") == 0) {
        print_usage();
        return;
    }

    if (strcmp(argv[1], "status") == 0) {
        print_status_cli();
    } else if (strcmp(argv[1], "monitor") == 0) {
        handle_monitor(argc, argv);
    } else if (strcmp(argv[1], "log") == 0) {
        handle_log(argc, argv);
    } else if (strcmp(argv[1], "event") == 0) {
        handle_event(argc, argv);
    } else {
        cliPrintf("unknown adas command\r\n");
        print_usage();
    }
}

static void ADAS_Cli_Process(void)
{
    uint32_t now;

    if (!s_monitor_enabled) {
        return;
    }

    now = osKernelGetTickCount();
    if ((uint32_t)(now - s_monitor_last_tick) < s_monitor_interval_ms) {
        return;
    }

    s_monitor_last_tick = now;
    print_monitor_line();
}

static void ADAS_Safety_Init(void)
{
    if (s_initialized) {
        return;
    }

    memset(&s_status, 0, sizeof(s_status));
    (void)uartInit();
    cliInit();
    (void)cliAdd("adas", cmd_adas);
    ADAS_Input_Init();
    if (s_input_task_handle == NULL) {
        s_input_task_handle = osThreadNew(ADAS_InputTask, NULL, &s_input_task_attributes);
    }
    (void)ADAS_Can_Init();
    s_initialized = 1U;

    uartPrintf(0, "[ADAS] Board E Safety ECU ready\r\n");
    uartPrintf(0, "[ADAS] logs are off. use 'adas status', 'adas monitor on 100', or 'adas log on'\r\n");
    uartPrintf(0, "CLI > ");
}

void ADAS_Safety_Task(void *argument)
{
    uint32_t now;
    uint32_t next_status_tick;
    uint32_t next_log_tick;
    uint8_t alive = 0U;

    (void)argument;
    ADAS_Safety_Init();

    now = osKernelGetTickCount();
    next_status_tick = now;
    next_log_tick = now + ADAS_LOG_PERIOD_MS;

    for (;;) {
        cliMain();
        ADAS_Cli_Process();
        ADAS_Can_PollRx(0U);
        now = osKernelGetTickCount();

        if ((uint32_t)(now - next_status_tick) < 0x80000000UL) {
            evaluate_status(alive++);
            (void)ADAS_Can_SendStatus(&s_status);
            log_status_if_needed(0U);
            next_status_tick = now + ADAS_STATUS_PERIOD_MS;
        }

        if ((uint32_t)(now - next_log_tick) < 0x80000000UL) {
            AdasCanStats_t stats;

            if (s_periodic_log_enabled) {
                ADAS_Can_GetStats(&stats);
                uartPrintf(0, "[ADAS] can tx=%lu rx=%lu err=%lu engine=%u speed=%u\r\n",
                           (unsigned long)stats.tx_count,
                           (unsigned long)stats.rx_count,
                           (unsigned long)stats.tx_error_count,
                           (unsigned int)stats.engine_active,
                           (unsigned int)stats.speed_kmh);
                uartPrintf(0, "[ADAS] verify 3A0_dt=%lums age=%lums rx_dt=%lums ign_age=%lums spd_age=%lums\r\n",
                           (unsigned long)stats.status_tx_period_ms,
                           (unsigned long)stats.status_tx_age_ms,
                           (unsigned long)stats.rx_period_ms,
                           (unsigned long)stats.ign_rx_age_ms,
                           (unsigned long)stats.speed_rx_age_ms);
                log_status_if_needed(1U);
            }
            next_log_tick = now + ADAS_LOG_PERIOD_MS;
        }

        osDelay(5U);
    }
}

static void ADAS_InputTask(void *argument)
{
    (void)argument;

    for (;;) {
        ADAS_Input_Poll();
        osDelay(ADAS_INPUT_PERIOD_MS);
    }
}

void EngineSim_Task(void *argument)
{
    ADAS_Safety_Task(argument);
}
