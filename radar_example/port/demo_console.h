#ifndef DEMO_CONSOLE_H
#define DEMO_CONSOLE_H

#include <stdbool.h>

void DemoConsole_Init(void);
void DemoConsole_Poll(void);
void DemoConsole_Printf(const char *format, ...);
bool DemoConsole_TakeStopRequest(void);
void DemoConsole_OnUartTxCompleteInterrupt(void);
void DemoConsole_OnUartErrorInterrupt(void);

#endif

