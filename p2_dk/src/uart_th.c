#include <ctype.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include "uart_th.h"

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif

LOG_MODULE_DECLARE(stage_2);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/**
 * Takes the attributes in the struct uart_msg and packs them into the array in uart_msg.msg
 * according to the HCI protocol.
 *
 * @param:
 *       um <uart_msg>: uart msg struct.
 */
void build_uart_msg(uart_msg *um) {

    memset(um->msg, 'a', 16);

    um->msg[0] = PREAMBLE;
    um->msg[1] = 0x10 | um->length;

    um->msg[2] = um->type;
    int i = 3;
    int x = 0;
    while (x < um->length) {

        um->msg[i] = um->data[x];
        um->msg[i + 1] = 'a';

        i++;
        x++;
    }

    // LOG_INF("X:%d\r\n", x);
    um->msg[15] = '\0';
}

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 *
 * @params:
 *      dev <const struct device>: Pointer to the UART device node.
 * 		user_data <void *>: Pointer to the data.
 */
void serial_cb(const struct device *dev, void *user_data) {
    uint8_t c;

    if (!uart_irq_update(uart_dev)) {
        return;
    }

    if (!uart_irq_rx_ready(uart_dev)) {
        return;
    }

    /* read until FIFO empty */
    while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
            /* terminate string */
            rx_buf[rx_buf_pos] = '\0';

            /* if queue is full, message is silently dropped */
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

            /* reset the buffer (it was copied to the msgq) */
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            rx_buf[rx_buf_pos++] = c;
        }
        /* else: characters beyond buffer size are dropped */
    }
}

/*
 * Print a null-terminated string character by character to the UART interface.
 *
 * @param str The string to print.
 */
void print_uart(char *buf) {

    int msg_len = MSG_SIZE;

    for (int i = 0; i < msg_len; i++) {
        uart_poll_out(uart_dev, buf[i]);
        // LOG_INF("i: %d: %02X\n", i, buf[i]);
    }
}

/**
 * Thread that sends messages to the UART interface.
 *
 * This thread waits for messages to arrive in the queue and then sends them using the Zephyr UART
 * drivers UART interface.
 */
int uart_send_th(void) {

    char tx_buf[MSG_SIZE];

    if (!device_is_ready(uart_dev)) {
        printk("UART device not found!");
        return 0;
    }

    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            printk("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            printk("UART device does not support interrupt-driven API\n");
        } else {
            printk("Error setting UART callback: %d\n", ret);
        }
        return 0;
    }
    // uart_irq_rx_enable(uart_dev);

    /* Wait to receiver something from the UART queue. */
    while (1) {

        if (k_msgq_get(&uart_msgq, &tx_buf, K_MSEC(100)) == 0) {
            print_uart(tx_buf);
        }
    }

    return 0;
}
