/**
 * @file    bcm_cli.h
 * @brief   UART CLI commands for the Board D Body / BCM simulator.
 */

#ifndef BCM_CLI_H
#define BCM_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

void BCM_Cli_Init(void);
void BCM_Cli_Process(void);
void BCM_Cli_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* BCM_CLI_H */
