#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "bluetooth_send_recv.h"
#include "main.h"
#include "shell_commands.h"
#include "uart_th.h"

#define NAME_LEN 30

#define MAJOR_1 25
#define MAJOR_2 MAJOR_1 + 1
#define MINOR_1 27
#define MINOR_2 MINOR_1 + 1

// #define SCAN_DURATION 1000 // Milliseconds

extern struct k_msgq uart_msgq;
struct k_msgq bt_data_msgq;

LOG_MODULE_DECLARE(stage_2);

K_MSGQ_DEFINE(send_data_msgq, sizeof(uint8_t), 10, 4);
K_MSGQ_DEFINE(bt_data_msgq, sizeof(uint8_t) * 4, 10, 4);

uint8_t filter_adv = 0;
static bt_addr_le_t filter_addr, filter_con;

/**
 * This function is a callback function for bt_le_scan_start function called from the blescan
 * function. It prints the information for all of the packets it receives.
 */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad) {
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    LOG_INF("Device found: %s (RSSI %d), type %u, AD data len %u\n", addr_str, rssi, type, ad->len);
}

/**
 * This function is a callback function for bt_le_scan_start function called from the blescan
 * function in filter mode. It filters for the given device address and then prints its information
 * to the shell.
 */
static void device_found_filter(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                                struct net_buf_simple *ad) {
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    /* Check if the source address of the received packet matches that address we are filtering for.
     */
    if (bt_addr_le_eq(addr, &filter_addr) == true) {
        LOG_INF("Device found: %s (RSSI %d), type %u, AD data len %u\n", addr_str, rssi, type,
                ad->len);
    }
}

/**
 * This function is a callback function for bt_le_scan_start function called from the blecon
 * function. It filters for the given device address and then read the major and minor bytes if the
 * packet is an iBeacon packet. Once is has read the data out it sends it to a queue.
 */
static void device_found_con(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                             struct net_buf_simple *ad) {
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    if (bt_addr_le_eq(addr, &filter_con) == true) {

        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

        /* Check if this packet is an iBeacon packet. */
        if ((ad->data[5] == 0x4c) && (ad->data[6] == 0x00) && (ad->data[7] == 0x02) &&
            (ad->data[8] == 0x15)) {

            uint8_t data[4];
            data[0] = ad->data[MAJOR_1];
            data[1] = ad->data[MAJOR_2];
            data[2] = ad->data[MINOR_1];
            data[3] = ad->data[MINOR_2];

            if (k_msgq_put(&bt_data_msgq, &data, K_MSEC(10)) != 0) {
                LOG_INF("Couldn't put data in bt_data_msgq\r\n");
            }
        }
    }
}

/**
 * This thread sends data to the GCU depending on the current device ID.
 */
int send_data_th(void) {

    uint8_t dev_id = 0;
    uint16_t temp_value = 0;
    uint16_t integers = 0;
    uint16_t decimal = 0;
    uint8_t recv_data[4];

    while (1) {

        if ((dev_id != DEV_ID_NONE) && (dev_id != DEV_ID_TVOC)) {
            integers = temp_value / 100;
            decimal = temp_value % 100;

            uart_msg um;
            um.type = DATA_SEND_TYPE;
            um.length = 8;
            uint8_t data[6] = {dev_id,
                               ((integers >> 8) & 0xFF) | (0x80),
                               (integers & 0xFF),
                               ((decimal >> 8) & 0xFF) | (0x80),
                               (decimal & 0xFF),
                               '\0'};
            um.data = data;
            build_uart_msg(&um);

            if (k_msgq_put(&uart_msgq, &um.msg, K_NO_WAIT) == 0) {
                // LOG_INF("DATA_SEND\tMSG put in queue\r\n");
            } else {
                LOG_ERR("DATA_SEND\tMSG failed to be put in queue\r\n");
            }
        } else if (dev_id == DEV_ID_TVOC) {
            integers = temp_value / 10;
            decimal = temp_value % 10;

            uart_msg um;
            um.type = DATA_SEND_TYPE;
            um.length = 8;
            uint8_t data[6] = {dev_id,
                               ((integers >> 8) & 0xFF) | (0x80),
                               (integers & 0xFF),
                               ((decimal >> 8) & 0xFF) | (0x80),
                               (decimal & 0xFF),
                               '\0'};
            um.data = data;
            build_uart_msg(&um);

            if (k_msgq_put(&uart_msgq, &um.msg, K_NO_WAIT) == 0) {
                // LOG_INF("DATA_SEND\tMSG put in queue\r\n");
            } else {
                LOG_ERR("DATA_SEND\tMSG failed to be put in queue\r\n");
            }
        }

        if (k_msgq_get(&send_data_msgq, &dev_id, K_MSEC(100)) == 0) {
            // LOG_INF("Recved Dev ID: %d\r\n", dev_id);
        }

        if (k_msgq_get(&bt_data_msgq, &recv_data, K_MSEC(10)) == 0) {
            if (dev_id == recv_data[0]) { // If the received data matches the current device id
                temp_value = (recv_data[2] << 8) | recv_data[3];

                LOG_INF("Received Bluetooth Packet: %d - %d\r\n", recv_data[0], temp_value);
                // LOG_INF("Integer Dec: %d Hex: %02X\r\n", integers, integers);
                // LOG_INF("Decimal Dec: %d Hex: %02X\r\n", decimal, decimal);
            }
        }

        k_sleep(K_MSEC(200));
    }
}

/**
 * Callback for the bluetooth 'blescan' shell command.
 */
void blescan_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 2) {
        if (strcmp(argv[1], "-s") == 0) {
            LOG_INF("btscan START\r\n");
            struct bt_le_scan_param scan_param = {
                .type = BT_LE_SCAN_TYPE_PASSIVE,
                .options = BT_LE_SCAN_OPT_NONE,
                .interval = BT_GAP_SCAN_SLOW_INTERVAL_1,
                .window = BT_GAP_SCAN_FAST_WINDOW,
            };
            int err = bt_le_scan_start(&scan_param, device_found);
            if (err) {
                printk("Start scanning failed (err %d)\n", err);
                return;
            }

            printk("Started scanning...\n");

        } else if (strcmp(argv[1], "-p") == 0) {

            int err = bt_le_scan_stop();
            if (err) {
                LOG_ERR("Scan stop failed (err %d)\n", err);
                return;
            }
            LOG_INF("btscan STOP\r\n");
        } else {
            LOG_WRN("Incorrect input arguments\r\n");
        }

    } else if (argc == 4) {
        if ((strcmp(argv[1], "-s") == 0) && (strcmp(argv[2], "-f") == 0)) {
            LOG_INF("btscan START with filter\r\n");
            struct bt_le_scan_param scan_param = {
                .type = BT_LE_SCAN_TYPE_PASSIVE,
                .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
                .interval = BT_GAP_SCAN_FAST_INTERVAL,
                .window = BT_GAP_SCAN_FAST_WINDOW,
            };
            int err;

            int ret = bt_addr_le_from_str(argv[3], "random", &filter_addr);

            if (ret) {
                printk("Invalid address (err %d)\n", -EINVAL);
                return;
            }

            err = bt_le_scan_start(&scan_param, device_found_filter);
            if (err) {
                printk("Start scanning failed (err %d)\n", err);
                return;
            }
            printk("Started scanning...\n");

            ret = bt_addr_le_from_str(argv[3], "random", &filter_addr);
        }

    } else {
        LOG_WRN("Incorrect input arguments\r\n");
    }

    return;
}

/**
 * Callback for the bluetooth 'bleinit' shell command.
 */
void bleinit_h(const struct shell *shell, size_t argc, char *argv[]) {

    int err = bt_enable(NULL);
    bt_set_name("Ya mums old nokia\r\n");
    if (err) {
        LOG_ERR("Bluetooth initialization failed (err %d)\n", err);
    }
}

/**
 * Callback for the bluetooth 'bleshutdown' shell command.
 */
void bleshutdown_h(const struct shell *shell, size_t argc, char *argv[]) {

    int err = bt_disable();
    if (err) {
        LOG_ERR("Bluetooth shutdown failed (err %d)\n", err);
    }

    uint8_t dev_id = GCU_STOP_DATA_SEND;
    if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
        // LOG_INF("Dev id from GCUREC\tMSG put in queue\r\n");
    }
}

/**
 * Callback for the bluetooth 'blecon' shell command.
 */
void blecon_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 3) {
        if ((strcmp(argv[1], "-s") == 0)) {
            struct bt_le_scan_param scan_param = {
                .type = BT_LE_SCAN_TYPE_PASSIVE,
                .options = BT_LE_SCAN_OPT_NONE,
                .interval = BT_GAP_SCAN_FAST_INTERVAL,
                .window = BT_GAP_SCAN_FAST_WINDOW,
            };
            int err;

            int ret = bt_addr_le_from_str(argv[2], "random", &filter_con);

            if (ret) {
                printk("Invalid address (err %d)\n", -EINVAL);
                return;
            }

            err = bt_le_scan_start(&scan_param, device_found_con);
            if (err) {
                printk("Start connection failed (err %d)\n", err);
                return;
            }
            shell_print(shell, "Starting Connection...\n");
        }
    } else if (argc == 2) {
        if ((strcmp(argv[1], "-p") == 0)) {
            int err = bt_le_scan_stop();
            if (err) {
                LOG_ERR("Connection close failed (err %d)\n", err);
                return;
            }

            uint8_t dev_id = GCU_STOP_DATA_SEND;
            if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
                // LOG_INF("Dev id from GCUREC\tMSG put in queue\r\n");
            }

            shell_print(shell, "Connection closed\r\n");
        }
    } else {
        LOG_WRN("Incorrect input arguments\r\n");
    }
}

/* Registering bluetooth related shell cmds */
SHELL_CMD_ARG_REGISTER(blescan, NULL,
                       "Scans for bluetooth devices\r\nFormat:\r\n blescan "
                       "<s/p> -[f filter: <BLE ADDR>]\r\n",
                       blescan_h, 2, 2);
SHELL_CMD_ARG_REGISTER(bleinit, NULL, "Initialise and start bluetooth\r\n", bleinit_h, 0, 0);
SHELL_CMD_ARG_REGISTER(bleshutdown, NULL, "Initialise and start bluetooth\r\n", bleshutdown_h, 0,
                       0);
SHELL_CMD_ARG_REGISTER(blecon, NULL, "Listens to advertisements from specified source\r\n",
                       blecon_h, 2, 1);