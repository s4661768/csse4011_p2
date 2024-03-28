#ifndef __UART_TH_H__
#define __UART_TH_H__

#include <stdlib.h>
#define UART_TH_PRIORITY 7
#define UART_TH_STACKSIZE KB(1)

#define MSG_SIZE 16

// Macro definitions for HCI communication protocol.
#define PREAMBLE 0xAA
#define REQUEST 0x01
#define RESPONSE 0x02
#define LEDS_TYPE 0x01

#define DAC_T 2
#define DAC_X 1
#define DAC_Y 2
#define LCP_TYPE 0x03
#define LCP_X 0x01
#define LCP_Y 0x02
#define CIRCLE_TYPE 0x04
#define POINT_TYPE 0x05
#define MOVE_CIRCLE_TYPE 0x06
#define DATA_SEND_TYPE 0x01
#define GCU_GRAPH_TYPE 0x02
#define GCU_METER_TYPE 0x03
#define GCU_NUMERIC_TYPE 0x04
#define GCU_SD_TYPE 0x05
#define GCU_STOP_DATA_SEND 0x00

/* Struct holding attributes for UART message to GCU */
typedef struct uart_msg {

    uint8_t type;
    uint8_t length;
    uint8_t *data;
    uint8_t msg[MSG_SIZE];

} uart_msg;

/**
 * Takes the attributes in the struct uart_msg and packs them into the array in uart_msg.msg
 * according to the HCI protocol.
 *
 * @param:
 *       um <uart_msg>: uart msg struct.
 */
void build_uart_msg(uart_msg *um);

/**
 * Thread that handles sending UART messages to the configured UART peripheral via a queue.
 */
int uart_send_th(void);

#endif