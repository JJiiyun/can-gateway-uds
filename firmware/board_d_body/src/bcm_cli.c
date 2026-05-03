/**
 * @file    bcm_cli.c
 * @brief   UART CLI commands for the Board D Body / BCM simulator.
 */

#include "bcm_cli.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bcm_body.h"
#include "bcm_input.h"
#include "cli.h"
#include "cmsis_os2.h"

#define BCM_MONITOR_DEFAULT_INTERVAL_MS 500U
#define BCM_MONITOR_MIN_INTERVAL_MS     100U

static bool s_monitor_enabled;
static uint32_t s_monitor_interval_ms = BCM_MONITOR_DEFAULT_INTERVAL_MS;
static uint32_t s_monitor_last_tick;

static uint32_t parse_number(const char *str)
{
    if (str == NULL) {
        return 0U;
    }
    return (uint32_t)strtoul(str, NULL, 0);
}

static bool parse_bool_arg(const char *str, uint8_t *out_value)
{
    if (str == NULL || out_value == NULL) {
        return false;
    }

    if (strcmp(str, "1") == 0 || strcmp(str, "on") == 0 ||
        strcmp(str, "true") == 0 || strcmp(str, "open") == 0) {
        *out_value = 1U;
        return true;
    }

    if (strcmp(str, "0") == 0 || strcmp(str, "off") == 0 ||
        strcmp(str, "false") == 0 || strcmp(str, "closed") == 0) {
        *out_value = 0U;
        return true;
    }

    return false;
}

static const char *ign_override_string(void)
{
    int8_t override = BCM_Body_GetIgnOverride();

    if (override < 0) {
        return "auto";
    }
    return override ? "on" : "off";
}

static void print_monitor_line(void)
{
    BcmInput_State_t input;

    BCM_Input_GetState(&input);

    /*
     * Qt GUI parser-friendly single-line format.
     * Example:
     * BODY mode=uart ign=1 fl=1 fr=0 rl=0 rr=0 left=1 right=0 high=1 fog=0 tx=1234 rx=12
     */
    cliPrintf("BODY mode=%s ign=%u fl=%u fr=%u rl=%u rr=%u left=%u right=%u high=%u fog=%u tx=%lu rx=%lu\r\n",
              BCM_Input_GetModeString(),
              (unsigned)BCM_Body_IsIgnOn(),
              (unsigned)input.door_fl,
              (unsigned)input.door_fr,
              (unsigned)input.door_rl,
              (unsigned)input.door_rr,
              (unsigned)input.turn_left_enabled,
              (unsigned)input.turn_right_enabled,
              (unsigned)input.high_beam,
              (unsigned)input.fog_light,
              (unsigned long)BCM_Body_GetTxCount(),
              (unsigned long)BCM_Body_GetRxCount());
}

static void print_status(void)
{
    BcmInput_State_t input;

    BCM_Input_GetState(&input);

    cliPrintf("\r\n[BCM Body Status]\r\n");
    cliPrintf("mode        : %s\r\n", BCM_Input_GetModeString());
    cliPrintf("ign         : %u (%s)\r\n",
              (unsigned)BCM_Body_IsIgnOn(),
              ign_override_string());
    cliPrintf("door        : FL=%u FR=%u RL=%u RR=%u\r\n",
              (unsigned)input.door_fl,
              (unsigned)input.door_fr,
              (unsigned)input.door_rl,
              (unsigned)input.door_rr);
    cliPrintf("turn        : left=%u right=%u\r\n",
              (unsigned)input.turn_left_enabled,
              (unsigned)input.turn_right_enabled);
    cliPrintf("lamp        : high=%u fog=%u\r\n",
              (unsigned)input.high_beam,
              (unsigned)input.fog_light);
    cliPrintf("tx_count    : %lu\r\n", (unsigned long)BCM_Body_GetTxCount());
    cliPrintf("rx_count    : %lu\r\n", (unsigned long)BCM_Body_GetRxCount());
    cliPrintf("monitor     : %s, interval=%lu ms\r\n",
              s_monitor_enabled ? "on" : "off",
              (unsigned long)s_monitor_interval_ms);
    cliPrintf("\r\n");
}

static void print_usage(void)
{
    cliPrintf("--------BCM Body Commands---------\r\n");
    cliPrintf("body mode gpio|uart\r\n");
    cliPrintf("body ign auto|on|off\r\n");
    cliPrintf("body door fl|fr|rl|rr|all <0|1>\r\n");
    cliPrintf("body turn left|right|both <0|1>\r\n");
    cliPrintf("body high <0|1>\r\n");
    cliPrintf("body fog <0|1>\r\n");
    cliPrintf("body all doors|lamps <0|1>\r\n");
    cliPrintf("body all off\r\n");
    cliPrintf("body reset\r\n");
    cliPrintf("body status\r\n");
    cliPrintf("body monitor on [interval_ms]\r\n");
    cliPrintf("body monitor off\r\n");
    cliPrintf("body monitor once\r\n");
    cliPrintf("----------------------------------\r\n");
}

static void handle_mode(uint8_t argc, char *argv[])
{
    if (argc < 3) {
        cliPrintf("mode = %s\r\n", BCM_Input_GetModeString());
        cliPrintf("usage: body mode gpio | uart\r\n");
        return;
    }

    if (strcmp(argv[2], "gpio") == 0) {
        BCM_Input_SetMode(BCM_INPUT_MODE_GPIO);
        cliPrintf("body mode = gpio\r\n");
    } else if (strcmp(argv[2], "uart") == 0) {
        BCM_Input_SetMode(BCM_INPUT_MODE_UART);
        cliPrintf("body mode = uart\r\n");
    } else {
        cliPrintf("invalid mode\r\n");
        cliPrintf("usage: body mode gpio | uart\r\n");
    }
}

static void handle_ign(uint8_t argc, char *argv[])
{
    if (argc < 3) {
        cliPrintf("ign = %u (%s)\r\n",
                  (unsigned)BCM_Body_IsIgnOn(),
                  ign_override_string());
        cliPrintf("usage: body ign auto | on | off\r\n");
        return;
    }

    if (strcmp(argv[2], "auto") == 0) {
        BCM_Body_SetIgnOverride(-1);
        cliPrintf("ign override = auto\r\n");
    } else if (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "1") == 0) {
        BCM_Body_SetIgnOverride(1);
        cliPrintf("ign override = on\r\n");
    } else if (strcmp(argv[2], "off") == 0 || strcmp(argv[2], "0") == 0) {
        BCM_Body_SetIgnOverride(0);
        cliPrintf("ign override = off\r\n");
    } else {
        cliPrintf("usage: body ign auto | on | off\r\n");
    }
}

static bool door_field_from_name(const char *name, BcmInput_Field_t *out_field)
{
    if (name == NULL || out_field == NULL) {
        return false;
    }

    if (strcmp(name, "fl") == 0 || strcmp(name, "front_left") == 0) {
        *out_field = BCM_INPUT_FIELD_DOOR_FL;
        return true;
    }
    if (strcmp(name, "fr") == 0 || strcmp(name, "front_right") == 0) {
        *out_field = BCM_INPUT_FIELD_DOOR_FR;
        return true;
    }
    if (strcmp(name, "rl") == 0 || strcmp(name, "rear_left") == 0) {
        *out_field = BCM_INPUT_FIELD_DOOR_RL;
        return true;
    }
    if (strcmp(name, "rr") == 0 || strcmp(name, "rear_right") == 0) {
        *out_field = BCM_INPUT_FIELD_DOOR_RR;
        return true;
    }

    return false;
}

static void handle_door(uint8_t argc, char *argv[])
{
    uint8_t value;

    if (argc < 4 || !parse_bool_arg(argv[3], &value)) {
        cliPrintf("usage: body door fl|fr|rl|rr|all <0|1>\r\n");
        return;
    }

    BCM_Input_SetMode(BCM_INPUT_MODE_UART);

    if (strcmp(argv[2], "all") == 0) {
        BCM_Input_SetAllDoors(value);
        cliPrintf("door all = %u\r\n", (unsigned)value);
        return;
    }

    BcmInput_Field_t field;

    if (!door_field_from_name(argv[2], &field)) {
        cliPrintf("invalid door selector\r\n");
        cliPrintf("usage: body door fl|fr|rl|rr|all <0|1>\r\n");
        return;
    }

    BCM_Input_SetField(field, value);
    cliPrintf("door %s = %u\r\n", argv[2], (unsigned)value);
}

static void handle_turn(uint8_t argc, char *argv[])
{
    uint8_t value;

    if (argc < 4 || !parse_bool_arg(argv[3], &value)) {
        cliPrintf("usage: body turn left|right|both <0|1>\r\n");
        return;
    }

    BCM_Input_SetMode(BCM_INPUT_MODE_UART);

    if (strcmp(argv[2], "left") == 0 || strcmp(argv[2], "l") == 0) {
        BCM_Input_SetField(BCM_INPUT_FIELD_TURN_LEFT, value);
        cliPrintf("turn left = %u\r\n", (unsigned)value);
    } else if (strcmp(argv[2], "right") == 0 || strcmp(argv[2], "r") == 0) {
        BCM_Input_SetField(BCM_INPUT_FIELD_TURN_RIGHT, value);
        cliPrintf("turn right = %u\r\n", (unsigned)value);
    } else if (strcmp(argv[2], "both") == 0 || strcmp(argv[2], "all") == 0) {
        BCM_Input_SetField(BCM_INPUT_FIELD_TURN_LEFT, value);
        BCM_Input_SetField(BCM_INPUT_FIELD_TURN_RIGHT, value);
        cliPrintf("turn both = %u\r\n", (unsigned)value);
    } else {
        cliPrintf("usage: body turn left|right|both <0|1>\r\n");
    }
}

static void handle_lamp_field(uint8_t argc,
                              char *argv[],
                              BcmInput_Field_t field,
                              const char *name)
{
    uint8_t value;

    if (argc < 3 || !parse_bool_arg(argv[2], &value)) {
        cliPrintf("usage: body %s <0|1>\r\n", name);
        return;
    }

    BCM_Input_SetMode(BCM_INPUT_MODE_UART);
    BCM_Input_SetField(field, value);
    cliPrintf("%s = %u\r\n", name, (unsigned)value);
}

static void handle_all(uint8_t argc, char *argv[])
{
    uint8_t value;

    if (argc >= 3 && strcmp(argv[2], "off") == 0) {
        BCM_Input_SetMode(BCM_INPUT_MODE_UART);
        BCM_Input_ClearAll();
        cliPrintf("body all = off\r\n");
        return;
    }

    if (argc < 4 || !parse_bool_arg(argv[3], &value)) {
        cliPrintf("usage: body all doors|lamps <0|1> | body all off\r\n");
        return;
    }

    BCM_Input_SetMode(BCM_INPUT_MODE_UART);

    if (strcmp(argv[2], "doors") == 0 || strcmp(argv[2], "door") == 0) {
        BCM_Input_SetAllDoors(value);
        cliPrintf("doors = %u\r\n", (unsigned)value);
    } else if (strcmp(argv[2], "lamps") == 0 || strcmp(argv[2], "lamp") == 0) {
        BCM_Input_SetAllLamps(value);
        cliPrintf("lamps = %u\r\n", (unsigned)value);
    } else {
        cliPrintf("usage: body all doors|lamps <0|1> | body all off\r\n");
    }
}

static void handle_monitor(uint8_t argc, char *argv[])
{
    if (argc < 3) {
        cliPrintf("usage: body monitor on [interval_ms] | off | once\r\n");
        cliPrintf("monitor = %s, interval = %lu ms\r\n",
                  s_monitor_enabled ? "on" : "off",
                  (unsigned long)s_monitor_interval_ms);
        return;
    }

    if (strcmp(argv[2], "on") == 0) {
        if (argc >= 4) {
            uint32_t interval = parse_number(argv[3]);

            if (interval < BCM_MONITOR_MIN_INTERVAL_MS) {
                interval = BCM_MONITOR_MIN_INTERVAL_MS;
            }
            s_monitor_interval_ms = interval;
        }

        s_monitor_last_tick = osKernelGetTickCount();
        s_monitor_enabled = true;
        cliPrintf("body monitor = on, interval = %lu ms\r\n",
                  (unsigned long)s_monitor_interval_ms);
    } else if (strcmp(argv[2], "off") == 0) {
        s_monitor_enabled = false;
        cliPrintf("body monitor = off\r\n");
    } else if (strcmp(argv[2], "once") == 0) {
        print_monitor_line();
    } else {
        cliPrintf("usage: body monitor on [interval_ms] | off | once\r\n");
    }
}

static void handle_reset(void)
{
    BCM_Input_SetMode(BCM_INPUT_MODE_GPIO);
    BCM_Input_ClearAll();
    BCM_Body_SetIgnOverride(-1);
    s_monitor_enabled = false;
    s_monitor_interval_ms = BCM_MONITOR_DEFAULT_INTERVAL_MS;
    cliPrintf("BCM body reset OK\r\n");
}

static void cmd_body(uint8_t argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        return;
    }

    if (strcmp(argv[1], "mode") == 0) {
        handle_mode(argc, argv);
    } else if (strcmp(argv[1], "ign") == 0) {
        handle_ign(argc, argv);
    } else if (strcmp(argv[1], "door") == 0) {
        handle_door(argc, argv);
    } else if (strcmp(argv[1], "turn") == 0) {
        handle_turn(argc, argv);
    } else if (strcmp(argv[1], "high") == 0) {
        handle_lamp_field(argc, argv, BCM_INPUT_FIELD_HIGH_BEAM, "high");
    } else if (strcmp(argv[1], "fog") == 0) {
        handle_lamp_field(argc, argv, BCM_INPUT_FIELD_FOG_LIGHT, "fog");
    } else if (strcmp(argv[1], "all") == 0) {
        handle_all(argc, argv);
    } else if (strcmp(argv[1], "reset") == 0) {
        handle_reset();
    } else if (strcmp(argv[1], "status") == 0) {
        print_status();
    } else if (strcmp(argv[1], "monitor") == 0) {
        handle_monitor(argc, argv);
    } else if (strcmp(argv[1], "help") == 0) {
        print_usage();
    } else {
        cliPrintf("unknown body command\r\n");
        print_usage();
    }
}

static void cmd_bcm_help(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    print_usage();
}

void BCM_Cli_Process(void)
{
    if (!s_monitor_enabled) {
        return;
    }

    uint32_t now = osKernelGetTickCount();

    if (now - s_monitor_last_tick < s_monitor_interval_ms) {
        return;
    }

    s_monitor_last_tick = now;
    print_monitor_line();
}

void BCM_Cli_Init(void)
{
    cliAdd("body", cmd_body);
    cliAdd("bcm_help", cmd_bcm_help);
}

void BCM_Cli_Task(void *argument)
{
    (void)argument;

    for (;;) {
        cliMain();
        BCM_Cli_Process();
        osDelay(1U);
    }
}
