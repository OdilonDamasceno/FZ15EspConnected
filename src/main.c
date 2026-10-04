/* ESP32-C3 / ESP-IDF 5.3.x. Generic UART RX capture, NOT a verified FZ15 decoder.
 * RXD from a verified 3.3 V interface -> GPIO5. No physical ESP TX connection.
 * TXD and enable/sleep of the transceiver must be held in the proper idle/normal
 * states by the interface hardware. This program does not configure those pins.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define SNIFF_UART UART_NUM_1
#define SNIFF_RX_GPIO 5
/* Reference from another Yamaha project; NOT confirmed for the FZ15. */
#define SNIFF_BAUD 16064
#define SNIFF_RX_INVERT false
#define RX_RING_SIZE (16 * 1024)
#define EVENT_QUEUE_LEN 64
#define CHUNK_SIZE 256

static QueueHandle_t uart_events;
static uint64_t total_bytes;
static uint64_t uart_errors;

static void report_uart_events(void)
{
    uart_event_t event;
    /* Bounded loop: reception must keep draining even under sustained errors. */
    for (int i = 0; i < EVENT_QUEUE_LEN; ++i)
    {
        if (xQueueReceive(uart_events, &event, 0) != pdTRUE)
        {
            break;
        }
        const char *name = NULL;
        switch (event.type)
        {
        case UART_FIFO_OVF:
            name = "FIFO_OVERFLOW_DATA_LOST";
            break;
        case UART_BUFFER_FULL:
            name = "RX_BUFFER_FULL_CAPTURE_AT_RISK";
            break;
        case UART_FRAME_ERR:
            name = "FRAME_ERROR_CHECK_BAUD_SIGNAL";
            break;
        case UART_PARITY_ERR:
            name = "PARITY_ERROR";
            break;
        case UART_BREAK:
            name = "BREAK_OR_LINE_HELD_LOW";
            break;
        default:
            break;
        }
        if (name != NULL)
        {
            ++uart_errors;
            printf("! t_read_us=%" PRId64 " %s\n", esp_timer_get_time(), name);
        }
    }
    /* Do not flush the ring buffer: retain bytes that are still available.
     * These notices are asynchronous; they cannot be assigned to one byte.
     */
}

static void print_chunk(const uint8_t *bytes, size_t count, int64_t read_time)
{
    static const char hex[] = "0123456789ABCDEF";
    char line[CHUNK_SIZE * 3 + 128];
    int header_len = snprintf(line, sizeof(line),
                              "RX t_read_us=%" PRId64 " offset=%" PRIu64 " n=%u :",
                              read_time, total_bytes, (unsigned)count);
    if (header_len < 0 || (size_t)header_len >= sizeof(line))
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    size_t pos = (size_t)header_len;
    for (size_t i = 0; i < count; ++i)
    {
        line[pos++] = ' ';
        line[pos++] = hex[bytes[i] >> 4];
        line[pos++] = hex[bytes[i] & 0x0F];
    }
    line[pos++] = '\n';
    if (fwrite(line, 1, pos, stdout) != pos)
    {
        /* Stop rather than silently pretend the host received a full capture. */
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    total_bytes += count;
}

void app_main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    const uart_config_t config = {
        .baud_rate = SNIFF_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(SNIFF_UART, RX_RING_SIZE, 0,
                                        EVENT_QUEUE_LEN, &uart_events, 0));
    ESP_ERROR_CHECK(uart_param_config(SNIFF_UART, &config));
    /* NO_CHANGE is not a hardware TX disconnect. Keep ESP TX physically
     * disconnected from the transceiver. No UART write API is called here.
     */
    ESP_ERROR_CHECK(uart_set_pin(SNIFF_UART, UART_PIN_NO_CHANGE, SNIFF_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_line_inverse(SNIFF_UART,
                                          SNIFF_RX_INVERT ? UART_SIGNAL_RXD_INV : 0));
    ESP_ERROR_CHECK(uart_set_rx_full_threshold(SNIFF_UART, 64));
    ESP_ERROR_CHECK(uart_set_rx_timeout(SNIFF_UART, 10));

    uint32_t actual_baud = 0;
    ESP_ERROR_CHECK(uart_get_baudrate(SNIFF_UART, &actual_baud));
    printf("# UART_RX_ONLY gpio=%d requested_baud=%d actual_baud=%" PRIu32
           " format=8N1 invert=%d\n",
           SNIFF_RX_GPIO, SNIFF_BAUD, actual_baud, SNIFF_RX_INVERT);
    printf("# RX lines are read chunks, NOT Yamaha frames. Times are read times.\n");

    uint8_t bytes[CHUNK_SIZE];
    int64_t last_status = esp_timer_get_time();
    for (;;)
    {
        /* Read irrespective of DATA events, so queue congestion alone does
         * not strand bytes in the RX ring buffer. No checksum/length filters.
         */
        const int n = uart_read_bytes(SNIFF_UART, bytes, sizeof(bytes),
                                      pdMS_TO_TICKS(20));
        const int64_t read_time = esp_timer_get_time();
        report_uart_events();
        if (n < 0)
        {
            ESP_ERROR_CHECK(ESP_FAIL);
        }
        if (n > 0)
        {
            print_chunk(bytes, (size_t)n, read_time);
        }
        if (read_time - last_status >= 5000000)
        {
            printf("# STATUS t_read_us=%" PRId64 " bytes=%" PRIu64
                   " uart_notices=%" PRIu64 "\n",
                   read_time, total_bytes, uart_errors);
            last_status = read_time;
        }
    }
}
