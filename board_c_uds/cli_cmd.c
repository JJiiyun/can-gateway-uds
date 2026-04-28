#include "uds_server.h"
#include "cli_cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define CLI_LINE_BUF_MAX 64

static char cli_line_buf[CLI_LINE_BUF_MAX];
static uint8_t cli_line_idx = 0;

void UDS_CLI_Init(void)
{
    cli_line_idx = 0;
    memset(cli_line_buf, 0, sizeof(cli_line_buf));
}

static void print_hex_response(const uds_resp_t *resp)
{
    uint8_t i;

    printf("[RESP] ");
    for (i = 0; i < resp->len; i++)
    {
        printf("%02X ", resp->data[i]);
    }
    printf("\r\n");
}

static bool parse_did(const char *str, uint16_t *did)
{
    char *endptr;
    unsigned long value = strtoul(str, &endptr, 16);

    if (*endptr != '\0')
        return false;

    if (value > 0xFFFF)
        return false;

    *did = (uint16_t)value;
    return true;
}

void CLI_ProcessLine(char *line)
{
    char *cmd;
    char *arg;
    uint16_t did;
    uint8_t req[8] = {0};
    uds_resp_t resp;

    cmd = strtok(line, " \r\n");
    if (cmd == NULL)
        return;

    if (strcmp(cmd, "read_did") == 0)
    {
        arg = strtok(NULL, " \r\n");
        if (arg == NULL)
        {
            printf("usage: read_did F40C\r\n");
            return;
        }

        if (!parse_did(arg, &did))
        {
            printf("invalid did\r\n");
            return;
        }

        req[0] = 0x22;
        req[1] = (uint8_t)((did >> 8) & 0xFF);
        req[2] = (uint8_t)(did & 0xFF);

        printf("[REQ ] %02X %02X %02X\r\n", req[0], req[1], req[2]);

        if (UDS_Server_Process(req, 3, &resp))
        {
            print_hex_response(&resp);
        }
        else
        {
            printf("UDS process fail\r\n");
        }
    }
    else
    {
        printf("unknown command\r\n");
    }
}

void UDS_CLI_ProcessChar(char ch)
{
    if (ch == '\r' || ch == '\n')
    {
        if (cli_line_idx > 0)
        {
            cli_line_buf[cli_line_idx] = '\0';
            printf("\r\n");
            CLI_ProcessLine(cli_line_buf);
            cli_line_idx = 0;
            memset(cli_line_buf, 0, sizeof(cli_line_buf));
        }
        printf("> ");
        return;
    }

    if (ch == '\b' || ch == 127)
    {
        if (cli_line_idx > 0)
        {
            cli_line_idx--;
            cli_line_buf[cli_line_idx] = '\0';
            printf("\b \b");
        }
        return;
    }

    if (cli_line_idx < (CLI_LINE_BUF_MAX - 1))
    {
        cli_line_buf[cli_line_idx++] = ch;
        printf("%c", ch);
    }
}