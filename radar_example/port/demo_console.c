#include "demo_console.h"

#include "radar_uart_port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef RADAR_DEBUG_UART_HANDLE

#define CONSOLE_RX_DMA_SIZE 64U
#define CONSOLE_COMMAND_SIZE 16U
#define CONSOLE_TX_DEPTH 32U
#define CONSOLE_LINE_SIZE 192U

typedef struct {
    uint16_t length;
    char text[CONSOLE_LINE_SIZE];
} console_line_t;

static uint8_t rx_dma[CONSOLE_RX_DMA_SIZE];
static uint16_t rx_read_position;
static char command[CONSOLE_COMMAND_SIZE];
static uint8_t command_length;
static bool stop_requested;
static console_line_t tx_queue[CONSOLE_TX_DEPTH];
static uint8_t tx_head;
static uint8_t tx_tail;
static bool tx_busy;
static volatile bool tx_complete_pending;
static volatile bool uart_error_pending;
static uint32_t tx_dropped_lines;

static bool console_available(void)
{
    return RADAR_DEBUG_UART_HANDLE != RADAR_UART_HANDLE;
}

static bool rx_active(void)
{
    return RADAR_DEBUG_UART_HANDLE->hdmarx != NULL &&
           RADAR_DEBUG_UART_HANDLE->RxState == HAL_UART_STATE_BUSY_RX &&
           HAL_DMA_GetState(RADAR_DEBUG_UART_HANDLE->hdmarx) ==
               HAL_DMA_STATE_BUSY;
}

static void start_rx(void)
{
    (void)HAL_UART_AbortReceive(RADAR_DEBUG_UART_HANDLE);
    __HAL_UART_CLEAR_OREFLAG(RADAR_DEBUG_UART_HANDLE);
    rx_read_position = 0U;
    if (HAL_UART_Receive_DMA(RADAR_DEBUG_UART_HANDLE, rx_dma,
                             sizeof(rx_dma)) == HAL_OK) {
        __HAL_DMA_DISABLE_IT(RADAR_DEBUG_UART_HANDLE->hdmarx, DMA_IT_HT);
        uart_error_pending = false;
    } else {
        uart_error_pending = true;
    }
}

static void service_tx(void)
{
    if (tx_complete_pending) {
        tx_complete_pending = false;
        if (tx_busy && tx_tail != tx_head)
            tx_tail = (uint8_t)((tx_tail + 1U) % CONSOLE_TX_DEPTH);
        tx_busy = false;
    }
    if (tx_busy || tx_tail == tx_head) return;
    if (HAL_UART_Transmit_IT(
            RADAR_DEBUG_UART_HANDLE,
            (uint8_t *)tx_queue[tx_tail].text,
            tx_queue[tx_tail].length) == HAL_OK) {
        tx_busy = true;
    } else {
        uart_error_pending = true;
    }
}

static void enqueue_drop_notice(void)
{
    uint8_t next;
    int length;
    uint32_t dropped;

    if (tx_dropped_lines == 0U) return;
    next = (uint8_t)((tx_head + 1U) % CONSOLE_TX_DEPTH);
    if (next == tx_tail) return;

    dropped = tx_dropped_lines;
    length = snprintf(tx_queue[tx_head].text, CONSOLE_LINE_SIZE,
                      "[console] dropped %lu log line(s)\r\n",
                      (unsigned long)dropped);
    if (length <= 0) return;
    if (length >= (int)CONSOLE_LINE_SIZE)
        length = (int)CONSOLE_LINE_SIZE - 1;
    tx_queue[tx_head].length = (uint16_t)length;
    tx_head = next;
    tx_dropped_lines -= dropped;
}

static void finish_command(void)
{
    command[command_length] = '\0';
    if (strcmp(command, "stop") == 0 || strcmp(command, "STOP") == 0)
        stop_requested = true;
    command_length = 0U;
}

static void consume_byte(uint8_t value)
{
    if (value == '\r' || value == '\n') {
        if (command_length > 0U) finish_command();
        return;
    }
    if (command_length + 1U < CONSOLE_COMMAND_SIZE)
        command[command_length++] = (char)value;
    else
        command_length = 0U;
}

static void service_rx(void)
{
    uint16_t write_position;
    uint16_t index;
    write_position = (uint16_t)(CONSOLE_RX_DMA_SIZE -
                     __HAL_DMA_GET_COUNTER(RADAR_DEBUG_UART_HANDLE->hdmarx));
    if (write_position >= CONSOLE_RX_DMA_SIZE) write_position = 0U;
    index = rx_read_position;
    while (index != write_position) {
        consume_byte(rx_dma[index]);
        index = (uint16_t)((index + 1U) % CONSOLE_RX_DMA_SIZE);
    }
    rx_read_position = write_position;
}

void DemoConsole_Init(void)
{
    if (!console_available()) return;
    command_length = 0U;
    stop_requested = false;
    tx_head = 0U;
    tx_tail = 0U;
    tx_busy = false;
    tx_complete_pending = false;
    uart_error_pending = false;
    tx_dropped_lines = 0U;
    start_rx();
    DemoConsole_Printf("\r\nRadar protocol v6 demo. Type stop to cancel.\r\n");
}

void DemoConsole_Poll(void)
{
    if (!console_available()) return;
    if (uart_error_pending || !rx_active()) start_rx();
    if (rx_active()) service_rx();
    service_tx();
    enqueue_drop_notice();
    service_tx();
}

void DemoConsole_Printf(const char *format, ...)
{
    uint8_t next;
    va_list args;
    int length;
    if (!console_available() || format == NULL) return;
    next = (uint8_t)((tx_head + 1U) % CONSOLE_TX_DEPTH);
    if (next == tx_tail) {
        ++tx_dropped_lines;
        return;
    }
    va_start(args, format);
    length = vsnprintf(tx_queue[tx_head].text, CONSOLE_LINE_SIZE,
                       format, args);
    va_end(args);
    if (length <= 0) return;
    if (length >= (int)CONSOLE_LINE_SIZE)
        length = (int)CONSOLE_LINE_SIZE - 1;
    tx_queue[tx_head].length = (uint16_t)length;
    tx_head = next;
    service_tx();
}

bool DemoConsole_TakeStopRequest(void)
{
    bool value = stop_requested;
    stop_requested = false;
    return value;
}

void DemoConsole_OnUartTxCompleteInterrupt(void)
{
    tx_complete_pending = true;
}

void DemoConsole_OnUartErrorInterrupt(void)
{
    uart_error_pending = true;
}

#else

void DemoConsole_Init(void) { }
void DemoConsole_Poll(void) { }
void DemoConsole_Printf(const char *format, ...) { (void)format; }
bool DemoConsole_TakeStopRequest(void) { return false; }
void DemoConsole_OnUartTxCompleteInterrupt(void) { }
void DemoConsole_OnUartErrorInterrupt(void) { }

#endif
