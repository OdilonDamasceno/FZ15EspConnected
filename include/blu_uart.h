#if !defined(_BLU_UART_H)
#define _BLU_UART_H

#include <ble_uart.h>

typedef enum {
    BLU_UART_OK = 0,
    BLU_UART_ERR = -1,
} blu_uart_status_t;

typedef struct {
    const char *device_name;
} blu_uart_config_t;

int blu_uart_init(const blu_uart_config_t *config);

#endif // _BLU_UART_H
