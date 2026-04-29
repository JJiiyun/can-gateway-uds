#include "cli_cmd.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "cmsis_os2.h"
#include "engine_sim.h"

/*
 * CLI 설계 기준
 *
 * status  : 사람이 터미널에서 한 번 확인하는 다줄 출력
 * monitor : Qt/로그/시리얼 파싱용 한 줄 주기 출력
 *
 * Qt 연동을 고려해서 monitor 출력은 한 줄 고정 포맷으로 유지한다.
 * 화면 클리어 방식은 사람이 보기엔 좋지만 Qt 파싱에는 불리하므로 사용하지 않는다.
 */

#define MONITOR_DEFAULT_INTERVAL_MS 500U
#define MONITOR_MIN_INTERVAL_MS     100U

static bool monitor_enabled = false;
static uint32_t monitor_interval_ms = MONITOR_DEFAULT_INTERVAL_MS;
static uint32_t monitor_last_tick = 0;

static uint32_t parseNumber(const char *str)
{
    if (str == NULL)
    {
        return 0;
    }

    return (uint32_t)strtoul(str, NULL, 0);
}

static uint32_t clampPercent(uint32_t value)
{
    if (value > 100U)
    {
        value = 100U;
    }

    return value;
}

static void CliCmd_PrintMonitorLine(void)
{
    EngineSimStatus_t status;

    EngineSim_GetStatus(&status);

    /*
     * Qt 파싱용 고정 포맷
     *
     * 예:
     * MON mode=uart throttle=50 brake=0 rpm=3200 speed=60 coolant=102 tx=1234
     */
    cliPrintf("MON mode=%s throttle=%u brake=%u rpm=%u speed=%u coolant=%u tx=%lu\r\n",
              EngineSim_GetModeString(status.mode),
              (unsigned int)status.throttle,
              (unsigned int)status.brake,
              (unsigned int)status.rpm,
              (unsigned int)status.speed,
              (unsigned int)status.coolant,
              (unsigned long)status.tx_count);
}

static void CliCmd_Mode(uint8_t argc, char *argv[])
{
    if (argc < 2)
    {
        cliPrintf("usage: mode adc | uart\r\n");
        return;
    }

    if (strcmp(argv[1], "adc") == 0)
    {
        EngineSim_SetMode(ENGINE_MODE_ADC);
        cliPrintf("mode = adc\r\n");
    }
    else if (strcmp(argv[1], "uart") == 0)
    {
        EngineSim_SetMode(ENGINE_MODE_UART);
        cliPrintf("mode = uart\r\n");
    }
    else
    {
        cliPrintf("invalid mode\r\n");
        cliPrintf("usage: mode adc | uart\r\n");
    }
}

static void CliCmd_Throttle(uint8_t argc, char *argv[])
{
    if (argc < 2)
    {
        cliPrintf("usage: throttle <0~100>\r\n");
        return;
    }

    uint32_t value = clampPercent(parseNumber(argv[1]));

    EngineSim_SetMode(ENGINE_MODE_UART);
    EngineSim_SetThrottle((uint8_t)value);

    cliPrintf("mode = uart\r\n");
    cliPrintf("throttle = %lu %%\r\n", (unsigned long)value);
}

static void CliCmd_SimReset(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    EngineSim_Reset();

    cliPrintf("EngineSim reset OK\r\n");
}

static void CliCmd_Brake(uint8_t argc, char *argv[])
{
    if (argc < 2)
    {
        cliPrintf("usage: brake <0~100>\r\n");
        return;
    }

    uint32_t value = clampPercent(parseNumber(argv[1]));

    EngineSim_SetMode(ENGINE_MODE_UART);
    EngineSim_SetBrake((uint8_t)value);

    cliPrintf("mode = uart\r\n");
    cliPrintf("brake = %lu %%\r\n", (unsigned long)value);
}

static void CliCmd_Pedal(uint8_t argc, char *argv[])
{
    if (argc < 3)
    {
        cliPrintf("usage: pedal <throttle 0~100> <brake 0~100>\r\n");
        cliPrintf("ex) pedal 70 0\r\n");
        cliPrintf("ex) pedal 20 50\r\n");
        return;
    }

    uint32_t throttle = clampPercent(parseNumber(argv[1]));
    uint32_t brake = clampPercent(parseNumber(argv[2]));

    EngineSim_SetMode(ENGINE_MODE_UART);
    EngineSim_SetThrottle((uint8_t)throttle);
    EngineSim_SetBrake((uint8_t)brake);

    cliPrintf("mode = uart\r\n");
    cliPrintf("throttle = %lu %%\r\n", (unsigned long)throttle);
    cliPrintf("brake    = %lu %%\r\n", (unsigned long)brake);
}

static void CliCmd_Stop(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    EngineSim_SetMode(ENGINE_MODE_UART);
    EngineSim_SetThrottle(0);
    EngineSim_SetBrake(0);

    cliPrintf("engine input reset\r\n");
}

static void CliCmd_Status(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    EngineSimStatus_t status;

    EngineSim_GetStatus(&status);

    cliPrintf("\r\n[EngineSim Status]\r\n");
    cliPrintf("mode     : %s\r\n", EngineSim_GetModeString(status.mode));
    cliPrintf("throttle : %u %%\r\n", (unsigned int)status.throttle);
    cliPrintf("brake    : %u %%\r\n", (unsigned int)status.brake);
    cliPrintf("rpm      : %u\r\n", (unsigned int)status.rpm);
    cliPrintf("speed    : %u\r\n", (unsigned int)status.speed);
    cliPrintf("coolant  : %u\r\n", (unsigned int)status.coolant);
    cliPrintf("tx_count : %lu\r\n", (unsigned long)status.tx_count);
    cliPrintf("\r\n");
}

static void CliCmd_Monitor(uint8_t argc, char *argv[])
{
    if (argc < 2)
    {
        cliPrintf("usage: monitor on [interval_ms] | off | once\r\n");
        cliPrintf("monitor = %s, interval = %lu ms\r\n",
                  monitor_enabled ? "on" : "off",
                  (unsigned long)monitor_interval_ms);
        return;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        if (argc >= 3)
        {
            uint32_t interval = parseNumber(argv[2]);

            if (interval < MONITOR_MIN_INTERVAL_MS)
            {
                interval = MONITOR_MIN_INTERVAL_MS;
            }

            monitor_interval_ms = interval;
        }

        monitor_last_tick = osKernelGetTickCount();
        monitor_enabled = true;

        cliPrintf("monitor = on, interval = %lu ms\r\n",
                  (unsigned long)monitor_interval_ms);
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        monitor_enabled = false;
        cliPrintf("monitor = off\r\n");
    }
    else if (strcmp(argv[1], "once") == 0)
    {
        CliCmd_PrintMonitorLine();
    }
    else
    {
        cliPrintf("usage: monitor on [interval_ms] | off | once\r\n");
    }
}

static void CliCmd_SimHelp(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    cliPrintf("--------EngineSim Commands--------\r\n");
    cliPrintf("mode adc\r\n");
    cliPrintf("mode uart\r\n");
    cliPrintf("throttle <0~100>\r\n");
    cliPrintf("brake <0~100>\r\n");
    cliPrintf("pedal <throttle> <brake>\r\n");
    cliPrintf("stop\r\n");
    cliPrintf("status\r\n");
    cliPrintf("monitor on [interval_ms]\r\n");
    cliPrintf("monitor off\r\n");
    cliPrintf("monitor once\r\n");
    cliPrintf("----------------------------------\r\n");

    cliPrintf("\r\n");
}

void CliCmd_Process(void)
{
    if (monitor_enabled == false)
    {
        return;
    }

    uint32_t now = osKernelGetTickCount();

    if (now - monitor_last_tick < monitor_interval_ms)
    {
        return;
    }

    monitor_last_tick = now;

    CliCmd_PrintMonitorLine();
}

void CliCmd_Init(void)
{
    cliAdd("sim_help", CliCmd_SimHelp);
    cliAdd("mode", CliCmd_Mode);
    cliAdd("throttle", CliCmd_Throttle);
    cliAdd("brake", CliCmd_Brake);
    cliAdd("pedal", CliCmd_Pedal);
    cliAdd("stop", CliCmd_Stop);
    cliAdd("status", CliCmd_Status);
    cliAdd("monitor", CliCmd_Monitor);
    cliAdd("sim_reset", CliCmd_SimReset);
}