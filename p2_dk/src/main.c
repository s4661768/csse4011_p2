/** @file
 *  @brief Interactive Bluetooth LE shell application
 *
 *  Application allows implement Bluetooth LE functional commands performing
 *  simple diagnostic interaction between LE host stack and LE controller
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <version.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/shell/shell.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/uuid.h>

#include "bluetooth_send_recv.h"
#include "main.h"
#include "shell_commands.h"
#include "uart_th.h"

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif

LOG_MODULE_REGISTER(stage_2);
extern struct k_msgq uart_msgq;

// Thread Macros
#define SHELL_TH_STACKSIZE KB(2)
#define CONTROLLER_TH_STACKSIZE KB(1)

#define SHELL_TH_PRIORITY 7
#define CONTROLLER_TH_PRIORITY 6

char LED_Info[5] = {'0', '0', '0', '0', '0'};
#define AHU_LED_QUEUE_LEN 1
#define AHU_LED_MSG_SIZE sizeof(LED_Info)
K_MSGQ_DEFINE(Ahu_led_msg_q, AHU_LED_MSG_SIZE, AHU_LED_QUEUE_LEN, 4);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec Leds[] = {led0, led1, led2, led3};

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME

/**
 * This threads initialises the board LEDs and sets them based on the received
 * shell command.
 */
int controller_th(void) {
    uint8_t led_states[] = {1, 1, 1, 1};

    int ret;
    if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1) || !gpio_is_ready_dt(&led2) ||
        !gpio_is_ready_dt(&led3)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    char l[] = {'0', '0', '0', '0', '0'};

    while (1) {
        if (k_msgq_get(&Ahu_led_msg_q, &l, K_TIMEOUT_ABS_TICKS(10)) == 0) {
            if (l[0] == '1') { // Set CMD
                for (int i = 0; i < 4; i++) {
                    if (l[i + 1] == '1') {        // Set LED
                        if (led_states[i] != 1) { // If LED isn't already on
                            gpio_pin_set_dt(&(Leds[i]), 1);
                            led_states[i] = 1;
                            LOG_INF("LED Set");
                        } else {
                            LOG_WRN("LED is already already on.");
                        }

                    } else {
                        if (led_states[i] != 0) {
                            gpio_pin_set_dt(&(Leds[i]), 0);
                            led_states[i] = 0;
                            LOG_INF("LED Cleared");
                        } else {
                            LOG_WRN("LED is already already off.");
                        }
                    }
                }

            } else {

                for (int i = 0; i < 4; i++) {
                    if (l[i + 1] == '1') { // Toggle LED

                        gpio_pin_toggle_dt(&(Leds[i]));
                        led_states[i] = (1 + led_states[i]) % 2;
                        LOG_INF("LED Toggle");
                    }
                }
            }
        }

        k_yield();
    }

    return 0;
}

K_THREAD_DEFINE(uart_send_th_id, UART_TH_STACKSIZE, uart_send_th, NULL, NULL, NULL,
                UART_TH_PRIORITY, 0, 0);
K_THREAD_DEFINE(controller_th_id, CONTROLLER_TH_STACKSIZE, controller_th, NULL, NULL, NULL,
                CONTROLLER_TH_PRIORITY, 0, 0);
K_THREAD_DEFINE(send_data_th_id, SEND_DATA_TH_STACKSIZE, send_data_th, NULL, NULL, NULL,
                SEND_DATA_TH_PRIORITY, 0, 0);