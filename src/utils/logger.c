#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "blu_uart.h"
#include "esp_err.h"
#include "esp_log.h"

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[256];
    va_list args_copy;
    va_copy(args_copy, args);
    vsnprintf(buffer, sizeof(buffer), format, args_copy);
    va_end(args_copy);

    ESP_LOGI("INFO", "%s", buffer);
    ble_uart_tx((const uint8_t *)buffer, strlen(buffer));

    va_end(args);
}