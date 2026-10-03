#include <blu_uart.h>
#include <esp_mac.h>
#include <unistd.h>
#include <esp_log.h>

static const char *TAG = "blu_uart";

static void ble_uart_on_rx(const uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "rx len: %u bytes", (unsigned)len);
    if (data == NULL || len == 0)
    {
        return;
    }
    ESP_LOG_BUFFER_HEX(TAG, data, len);
    ble_uart_tx(data, len); /* echo back */
}

static void ble_uart_on_event(const ble_uart_evt_t *e)
{
    switch (e->id)
    {
    case BLE_UART_EVT_CONNECTED:
    {
        const uint8_t *b = e->connected.peer.bytes;
        ESP_LOGI(TAG,
                 "evt: connected peer=%02x:%02x:%02x:%02x:%02x:%02x type=%u",
                 b[0], b[1], b[2], b[3], b[4], b[5], e->connected.peer.type);
        break;
    }
    case BLE_UART_EVT_DISCONNECTED:
        ESP_LOGI(TAG, "evt: disconnected reason=0x%x",
                 e->disconnected.reason);
        break;
    case BLE_UART_EVT_SUBSCRIBED:
        ESP_LOGI(TAG, "evt: %ssubscribed",
                 e->subscribed.subscribed ? "" : "un");
        break;
    case BLE_UART_EVT_LINK_SECURE:
        ESP_LOGI(TAG, "evt: link_secure enc=%d auth=%d bond=%d ks=%u",
                 e->link_secure.encrypted, e->link_secure.authenticated,
                 e->link_secure.bonded, e->link_secure.key_size);
        break;
    case BLE_UART_EVT_PASSKEY_DISPLAY:
        ESP_LOGI(TAG, "evt: passkey=%06" PRIu32, e->passkey.passkey);
        break;
    case BLE_UART_EVT_PASSKEY_REQUEST:
        /* Fires only when cfg.security.io_cap is KEYBOARD_ONLY or
         * KEYBOARD_DISPLAY (this example leaves io_cap at AUTO →
         * DisplayOnly, so it should not fire). For a real keypad
         * product, prompt the user for the 6 digits the central
         * displayed and feed them in:
         *
         *     ble_uart_passkey_reply(digits);
         *
         * See PORTING.md §5.6.1 for the full pattern. */
        ESP_LOGW(TAG, "evt: passkey entry requested — no UI wired in this "
                      "example (see PORTING.md §5.6.1)");
        break;
    case BLE_UART_EVT_NUMERIC_COMPARE:
        /* Fires only when cfg.security.io_cap is DISPLAY_YES_NO or
         * KEYBOARD_DISPLAY (likewise dormant in this example). For a
         * product with a yes/no control, surface the digits to the
         * user and resolve the comparison:
         *
         *     ble_uart_compare_reply(user_says_match);
         *
         * See PORTING.md §5.6.1. */
        ESP_LOGW(TAG, "evt: numeric compare %06" PRIu32 " — no yes/no UI wired (see PORTING.md §5.6.1)",
                 e->numeric_compare.passkey);
        break;
    case BLE_UART_EVT_PAIRING_FAILED:
        ESP_LOGW(TAG, "evt: pairing failed reason=0x%x",
                 e->pairing_failed.reason);
        break;
    case BLE_UART_EVT_CLOSED:
        /* Only after ble_uart_close_async(). This example does not use
         * close_async; do not ble_uart_uninstall() here — defer to an
         * app task (PORTING.md §5.3.2). Kept for -Wswitch. */
        if (e->closed.status == BLE_UART_OK)
        {
            ESP_LOGI(TAG, "evt: closed (async-close succeeded)");
        }
        else
        {
            ESP_LOGW(TAG, "evt: closed async-close failed status=%d",
                     e->closed.status);
        }
        break;
    }
}

int blu_uart_init(const blu_uart_config_t *config)
{
    uint8_t mac[6] = {0};
    esp_err_t mac_err = esp_read_mac(mac, ESP_MAC_BT);

    if (mac_err != ESP_OK)
    {
        ESP_LOGW(TAG, "esp_read_mac(BT) failed (%s); device name suffix will be 0000",
                 esp_err_to_name(mac_err));
    }
    char name[BLE_UART_DEVICE_NAME_MAX + 1];

    snprintf(name, sizeof(name), "%s-%02X%02X",
             config->device_name, mac[4], mac[5]);

    ESP_ERROR_CHECK(ble_uart_install(&(ble_uart_config_t){
        .encrypted = true,
        .device_name = name,
        .ble_uart_on_rx = ble_uart_on_rx,
        .on_event = ble_uart_on_event,
    }));

    ESP_ERROR_CHECK(ble_uart_open());

    return BLU_UART_OK;
}
