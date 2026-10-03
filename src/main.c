#include <esp_err.h>
#include <blu_uart.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/uart.h>
#include "logger.h"

#define KLINE_UART UART_NUM_1
#define KLINE_RX 5

static const char *TAG = "KLINE";

// Hipótese inicial de captura, não confirmada para sua ECU.
#define KLINE_BAUD 10400

void app_main()
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    blu_uart_init(&(blu_uart_config_t){
        .device_name = "FZ15",
    });

    const uart_config_t config = {
        .baud_rate = KLINE_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        KLINE_UART,
        4096, // Buffer de recepção
        0,    // Sem buffer de transmissão
        0,
        NULL,
        0));

    ESP_ERROR_CHECK(uart_param_config(KLINE_UART, &config));

    ESP_ERROR_CHECK(uart_set_pin(
        KLINE_UART,
        UART_PIN_NO_CHANGE, // Não conectar TX neste teste
        KLINE_RX,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));

    log_info("K-Line UART initialized on RX pin %d with baud rate %d", KLINE_RX, KLINE_BAUD);

    uint8_t buffer[256];

    while (1)
    {
        int length = uart_read_bytes(
            KLINE_UART,
            buffer,
            sizeof(buffer),
            pdMS_TO_TICKS(100));

        if (length > 0)
        {
            ESP_LOG_BUFFER_HEX_LEVEL(
                TAG, buffer, length, ESP_LOG_INFO);

            log_info("Received %d bytes: ", length);
        }
    }
}