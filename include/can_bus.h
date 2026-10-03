#if !defined(_CAN_BUS_H)
#define _CAN_BUS_H

#include <mcp2515.h>

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_INTERRUPT 22

typedef enum {
    CAN_BUS_OK = 0,
    CAN_BUS_ERR = -1,
} can_bus_status_t;

typedef struct {
    CAN_SPEED_t canSpeed;
    CAN_CLOCK_t canClock;
} can_bus_config_t;

int can_bus_init(const can_bus_config_t *config);

#endif // _CAN_BUS_H
