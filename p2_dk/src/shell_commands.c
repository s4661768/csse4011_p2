#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include "shell_commands.h"
#include "main.h"
#include "uart_th.h"

LOG_MODULE_DECLARE(stage_2);
extern char LED_Info[]; // TODO fix this to not be a global
extern struct k_msgq uart_msgq;
extern struct k_msgq send_data_msgq;
extern struct k_msgq Ahu_led_msg_q;

// 						DEFINING CUSTOM SHELL COMMANDS
int led_set(const struct shell *shell, size_t argc, char *argv[]) {

    if (strlen(argv[1]) != 4) {
        shell_print(shell, "Incorrect argument. Bitmask must be 4 bits long and can only contains "
                           "1s and 0s-> <0101>.\r\n");
        return 0;
    }

    /* Checking the bit mask consists of only 1s and 0s. */
    for (int i = 0; i < 4; i++) {
        if ((argv[1][i] != '0') && (argv[1][i] != '1')) {
            shell_print(shell, "Incorrect argument. Bitmask must be 4 bits long and can only "
                               "contains 1s and 0s-> <0101>.\r\n");
            return 0;
        }
    }

    LED_Info[0] = '1';
    LED_Info[1] = argv[1][0];
    LED_Info[2] = argv[1][1];
    LED_Info[3] = argv[1][2];
    LED_Info[4] = argv[1][3];
    k_msgq_put(&Ahu_led_msg_q, LED_Info, K_MSEC(50));
    return 0;
}

int led_toggle(const struct shell *shell, size_t argc, char *argv[]) {

    if (strlen(argv[1]) != 4) {
        shell_print(shell, "Incorrect argument. Bitmask must be 4 bits long and can only contains "
                           "1s and 0s-> <0101>.\r\n");
        return 0;
    }

    /* Checking the bit mask consists of only 1s and 0s. */
    for (int i = 0; i < 4; i++) {
        if ((argv[1][i] != '0') && (argv[1][i] != '1')) {
            shell_print(shell, "Incorrect argument. Bitmask must be 4 bits long and can only "
                               "contains 1s and 0s-> <0101>.\r\n");
            return 0;
        }
    }

    LED_Info[0] = '0';
    LED_Info[1] = argv[1][0];
    LED_Info[2] = argv[1][1];
    LED_Info[3] = argv[1][2];
    LED_Info[4] = argv[1][3];
    k_msgq_put(&Ahu_led_msg_q, LED_Info, K_MSEC(50));
    return 0;
}

int system_timer_read(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 1) {
        int64_t tot_sec = k_cyc_to_ns_floor64(sys_clock_tick_get()) / NSEC_PER_MSEC / MSEC_PER_SEC;
        shell_fprintf(shell, SHELL_INFO, "%lld Seconds\r\n", tot_sec);

    } else if (argc == 2) {
        if ((strcmp(argv[1], "f")) == 0) {
            int64_t tot_sec =
                k_cyc_to_ns_floor64(sys_clock_tick_get()) / NSEC_PER_MSEC / MSEC_PER_SEC;
            int64_t tot_min = tot_sec / SEC_PER_MIN;
            int64_t min = tot_min % MIN_PER_HOUR;
            int64_t hour = tot_min / MIN_PER_HOUR;
            shell_fprintf(shell, SHELL_INFO, "%lld:%lld:%lld\r\n", hour, min, tot_sec % 60);
        }
    }

    return 0;
}

int point_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 3) {
        LOG_DBG("Point cmd called.\r\n");

        uint8_t num1, num2;
        num1 = atoi(argv[1]);
        num2 = atoi(argv[2]);

        /* Error Checking. */
        if ((num1 > 33 || num1 < 0) || (num2 > 33 || num2 < 0)) {
            LOG_WRN("Incorrect arguments. Arguments out of range.\r\n");
            return 0;
        }

        uart_msg um;

        um.type = POINT_TYPE;
        um.length = 3;
        uint8_t data[3] = {num1, num2, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_NO_WAIT) == 0) {
            LOG_INF("POINT\tMSG put in queue\r\n");
        } else {
            LOG_ERR("POINT\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect Point cmd format.\r\n");
    }

    return 0;
}

int move_c_h(const struct shell *shell, size_t argc, char *argv[]) {
    if (argc == 3) {
        LOG_DBG("MOVE_CIRCLE cmd called.\r\n");

        uint8_t num1, num2;
        num1 = atoi(argv[1]);
        num2 = atoi(argv[2]);

        /* Error Checking. */
        if ((num1 > 33 || num1 < 0) || (num2 > 33 || num2 < 0)) {
            LOG_WRN("Incorrect arguments. Arguments out of range.\r\n");
            return 0;
        }

        uart_msg um;

        um.type = MOVE_CIRCLE_TYPE;
        um.length = 3;
        uint8_t data[3] = {num1, num2, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_NO_WAIT) == 0) {
            LOG_INF("MOVE_CIRCLE\tMSG put in queue\r\n");
        } else {
            LOG_ERR("MOVE_CIRCLE\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect Point cmd format.\r\n");
    }

    return 0;
}

int circle_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 4) {
        LOG_DBG("Circle cmd called.\r\n");

        uint8_t num1, num2, num3;
        num1 = atoi(argv[1]);
        num2 = atoi(argv[2]);
        num3 = atoi(argv[3]);

        /* Error Checking. */
        if ((num1 > 33 || num1 < 0) || (num2 > 33 || num2 < 0) || (num3 > 33 || num3 < 0)) {
            LOG_WRN("Incorrect arguments. Arguments out of range.\r\n");
            return 0;
        }

        uart_msg um;

        um.type = CIRCLE_TYPE;
        um.length = 4;
        uint8_t data[4] = {num1, num2, num3, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_NO_WAIT) == 0) {
            LOG_INF("CIRCLE\tMSG put in queue\r\n");
        } else {
            LOG_ERR("CIRCLE\tMSG failed to be put in queue\r\n");
        }
    } else {
        shell_print(shell, "Incorrect Circle cmd format.\r\n");
    }

    return 0;
}

int lissajous_h(const struct shell *shell, size_t argc, char *argv[]) {
    if (argc == 5) {
        uint8_t num1, num2;
        uint32_t temp_num3;

        num1 = atoi(argv[2]);
        num2 = atoi(argv[3]);
        temp_num3 = atoi(argv[4]);

        /* Error Checking. */
        if ((num1 > 33 || num1 < 0) || (num2 > 33 || num2 < 0) ||
            (temp_num3 > 65536 || temp_num3 < 0)) {
            LOG_WRN("Incorrect arguments. Arguments out of range.\r\n");
            return 0;
        } else if ((strcmp(argv[1], "x") != 0) && (strcmp(argv[1], "y") != 0)) {
            LOG_WRN("Incorrect arguments. First argument should be lowercase 'x' or "
                    "'y'.\r\n");
            return 0;
        }

        uint16_t num3 = (temp_num3 & 0xFFFF);

        uart_msg um;
        um.type = LCP_TYPE;
        um.length = 6;
        uint8_t xy = 2;
        if (strcmp(argv[1], "x") == 0) {
            xy = LCP_X;
        } else {
            xy = LCP_Y;
        }
        LOG_INF("Sinusoid:%d\r\nNUM1:%d\r\nNUM2:%d\r\nNUM3:%d", xy, num1, num2, num3);

        /* Masking and shifting 16 bit number into 2 8 bit numbers. */
        uint8_t period_upper = (num3 >> 8) & 0xFF;
        uint8_t period_lower = num3 & 0xFF;
        LOG_INF("upper:%d\r\nlower:%d\r\n", period_upper, period_lower);

        uint8_t data[6] = {xy, num1, num2, period_upper, period_lower, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(10)) == 0) {
            LOG_INF("LJ\tMSG put in queue\r\n");
        } else {
            LOG_INF("LJ\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect Lissajous cmd format.\r\n");
    }

    return 0;
}

int dac_h(const struct shell *shell, size_t argc, char *argv[]) {
    uint8_t num1, num2;

    if (argc == 3) {

        num1 = atoi(argv[1]);
        num2 = atoi(argv[2]);
        if ((num1 > 33 || num1 < 0) || (num2 > 33 || num2 < 0)) {
            LOG_WRN("Incorrect arguments. Arguments out of range.\r\n");
            return 0;
        }

        /* Make and send message to set DAC X voltage. */
        uart_msg um;
        um.type = 2;
        um.length = 3;
        uint8_t data[3] = {DAC_X, num1, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(10)) == 0) {
            LOG_INF("DAC\tMSG put in queue\r\n");
        } else {
            LOG_INF("DAC\tMSG failed to be put in queue\r\n");
        }

        /* Make and send message to set DAC Y voltage. */
        uint8_t data2[3] = {DAC_Y, num2, '\0'};
        uart_msg um2;
        um2.type = 2;
        um2.length = 3;
        um2.data = data2;

        if (k_msgq_put(&uart_msgq, &um2.msg, K_MSEC(10)) == 0) {
            LOG_INF("DAC\tMSG put in queue\r\n");
        } else {
            LOG_INF("DAC\tMSG failed to be put in queue\r\n");
        }

    } else {
        LOG_WRN("Incorrect DAC cmd format.\r\n");
    }

    return 0;
}

int gled_h(const struct shell *shell, size_t argc, char *argv[]) {
    if (argc == 4) {
        uint8_t num = 0;
        for (int i = 1; i < 4; i++) {
            if ((strcmp(argv[i], "1") != 0) && (strcmp(argv[i], "0") != 0)) {
                LOG_INF("Arg: %s\t%d\r\n", argv[i], i);
                shell_print(shell, "Incorrect GCU LED cmd format. Only takes in 0 and 1.\r\n");
                return 0;
            }

            if (strcmp(argv[i], "1") == 0) {
                switch (i) {

                case 1:
                    num |= (1 << 2);
                    break;

                case 2:
                    num |= (1 << 1);
                    break;

                case 3:
                    num |= (1 << 0);
                    break;
                }
            }
        }

        LOG_INF("Num: %d\r\n", num);

        uint8_t data[3] = {1, num, '\0'};
        uart_msg um;
        um.type = LEDS_TYPE;
        um.length = 3;
        um.data = data; // Set mode
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(40)) == 0) {
            LOG_INF("GLED\tMSG put in queue\r\n");
        } else {
            LOG_INF("GLED\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect GCU LED cmd format.\r\n");
    }

    return 0;
}

int gpd_h(const struct shell *shell, size_t argc, char *argv[]) {
    if (argc == 1) {
        uart_msg um;

        build_uart_msg(&um);
    } else {
        shell_print(shell, "Incorrect GPD cmd format.\r\n");
    }

    return 0;
}

int gcugraph_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 2) {
        LOG_DBG("GCU GRAPH cmd called.\r\n");

        uint8_t dev_id = atoi(argv[1]);

        /* Error Checking. */
        if ((dev_id > 8 || dev_id < 5 || dev_id == 0)) {
            LOG_WRN("Invalid device ID. Valid device IDs are:\r\n5 -> Temperature\r\n6 "
                    "-> Humidity\r\n7 -> Air pressure\r\n8 -> TVOC\r\n");
            return 0;
        }

        uart_msg um;

        um.type = GCU_GRAPH_TYPE;
        um.length = 2;
        uint8_t data[2] = {1, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(40)) == 0) {
            LOG_INF("GCUGRAPH\tMSG put in queue\r\n");
        } else {
            LOG_ERR("GCUGRAPH\tMSG failed to be put in queue\r\n");
        }

        if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
            LOG_INF("Dev id from GCUGRAPH\tMSG put in queue\r\n");
        } else {
            LOG_ERR("Dev id from GCUGRAPH\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect GCUGRAPH cmd format.\r\n");
    }

    return 0;
}

int gcumeter_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 2) {
        LOG_DBG("GCUMETER cmd called.\r\n");

        uint8_t dev_id = atoi(argv[1]);

        /* Error Checking. */
        if ((dev_id > 8 || dev_id < 5 || dev_id == 0)) {
            LOG_WRN("Invalid device ID. Valid device IDs are:\r\n5 -> Temperature\r\n6 "
                    "-> Humidity\r\n7 -> Air pressure\r\n8 -> TVOC\r\n");
            return 0;
        }

        uart_msg um;

        um.type = GCU_METER_TYPE;
        um.length = 2;
        uint8_t data[2] = {1, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(40)) == 0) {
            LOG_INF("GCUMETER\tMSG put in queue\r\n");
        } else {
            LOG_ERR("GCUMETER\tMSG failed to be put in queue\r\n");
        }

        if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
            LOG_INF("Dev id from GCUMETER\tMSG put in queue\r\n");
        } else {
            LOG_ERR("Dev id from GCUMETER\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect GCUMETER cmd format.\r\n");
    }

    return 0;
}

int gcunumeric_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 2) {
        LOG_DBG("GCUNUMERIC cmd called.\r\n");

        uint8_t dev_id = atoi(argv[1]);

        /* Error Checking. */
        if ((dev_id > 8 || dev_id < 5 || dev_id == 0)) {
            LOG_WRN("Invalid device ID. Valid device IDs are:\r\n5 -> Temperature\r\n6 "
                    "-> Humidity\r\n7 -> Air pressure\r\n8 -> TVOC\r\n");
            return 0;
        }

        uart_msg um;

        um.type = GCU_NUMERIC_TYPE;
        um.length = 2;
        uint8_t data[2] = {1, '\0'};
        um.data = data;
        build_uart_msg(&um);

        if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(40)) == 0) {
            LOG_INF("GCUNUMERIC\tMSG put in queue\r\n");
        } else {
            LOG_ERR("GCUNUMERIC\tMSG failed to be put in queue\r\n");
        }

        if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
            LOG_INF("Dev id from GCUNUMERIC\tMSG put in queue\r\n");
        } else {
            LOG_ERR("Dev id from GCUNUMERIC\tMSG failed to be put in queue\r\n");
        }

    } else {
        shell_print(shell, "Incorrect GCUNUMERIC cmd format.\r\n");
    }

    return 0;
}

int gcurec_h(const struct shell *shell, size_t argc, char *argv[]) {

    if (argc == 4) {
        if (strcmp(argv[1], "-s") == 0) {
            LOG_INF("GCUREC Start cmd called.\r\n");

            uint8_t dev_id = atoi(argv[2]);

            /* Error Checking. */
            if ((dev_id > 8 || dev_id < 5)) {
                LOG_WRN("Invalid device ID. Valid device IDs are:\r\n5 -> Temperature\r\n6 "
                        "-> Humidity\r\n7 -> Air pressure\r\n8 -> TVOC\r\n");
                return 0;
            }

            if ((strlen(argv[3]) != 9) || (argv[3][5] != '.')) {
                LOG_WRN("Invalid file name. Valid file name example is \"abcde.csv\"\r\n");
                return 0;
            }

            // LOG_INF("Dev_id: %d\r\n", dev_id);
            // LOG_INF("File Name: %s\r\n", argv[3]);

            uart_msg um;

            um.type = GCU_SD_TYPE;
            um.length = 11;
            uint8_t data[11];
            data[0] = 1;
            memcpy(data + 1, argv[3], strlen(argv[3]) + 1);
            um.data = data;
            build_uart_msg(&um);

            if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(40)) == 0) {
                // LOG_INF("GCUREC\tMSG put in queue\r\n");
            } else {
                LOG_ERR("GCUREC\tMSG failed to be put in queue\r\n");
            }

            if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
                // LOG_INF("Dev id from GCUREC\tMSG put in queue\r\n");
            } else {
                LOG_ERR("Dev id from GCUREC\tMSG failed to be put in queue\r\n");
            }
        }

    } else if (argc == 2) {

        if (strcmp(argv[1], "-p") == 0) {
            LOG_INF("GCUREC Stop cmd called.\r\n");
            uart_msg um;

            um.type = GCU_SD_TYPE;
            um.length = 2;
            uint8_t data[2] = {0, '\0'};
            um.data = data;
            build_uart_msg(&um);
            uint8_t dev_id = GCU_STOP_DATA_SEND;

            if (k_msgq_put(&uart_msgq, &um.msg, K_MSEC(40)) == 0) {
                // LOG_INF("GCUREC\tMSG put in queue\r\n");
            } else {
                LOG_ERR("GCUREC\tMSG failed to be put in queue\r\n");
            }

            if (k_msgq_put(&send_data_msgq, &dev_id, K_MSEC(40)) == 0) {
                // LOG_INF("Dev id from GCUREC\tMSG put in queue\r\n");
            } else {
                LOG_ERR("Dev id from GCUREC\tMSG failed to be put in queue\r\n");
            }
        }

    } else {
        shell_print(shell, "Incorrect gcurec cmd format.\r\n");
    }

    return 0;
}

/* GCU shell command definitions. */
SHELL_CMD_ARG_REGISTER(point, NULL, "Draw point at [x, y]\r\nExample: point 2 10", point_h, 3, 0);
SHELL_CMD_ARG_REGISTER(move_c, NULL, "Move the current circle to [x, y]\r\nExample: move_c 5 15",
                       move_c_h, 3, 0);
SHELL_CMD_ARG_REGISTER(circle, NULL,
                       "Draw a circle with radius r with a centre at [x, "
                       "y].\r\nExample: circle 4 2 10",
                       circle_h, 4, 0);
SHELL_CMD_ARG_REGISTER(lj, NULL, LJ_CMD_HELP_MSG, lissajous_h, 5, 0);
SHELL_CMD_ARG_REGISTER(dac, NULL, DAC_CMD_HELP_MSG, dac_h, 3, 0);
SHELL_CMD_ARG_REGISTER(gled, NULL, "Toggle the GCU user LEDs.\r\nExample: gled 1 0 1", gled_h, 4,
                       0);
SHELL_CMD_ARG_REGISTER(gpd, NULL, "Read GCU pushbutton state (0, or 1).\r\nExample: gpd", gpd_h, 1,
                       0);
SHELL_CMD_ARG_REGISTER(time, NULL, "Get system time formatted as HH:MM:SS", system_timer_read, 1,
                       1);
SHELL_CMD_ARG_REGISTER(gcugraph, NULL, "Displays the selected sensor values as a graph plot",
                       gcugraph_h, 1, 1);
SHELL_CMD_ARG_REGISTER(gcumeter, NULL, "Displays the selected sensor values as a meter widget",
                       gcumeter_h, 1, 1);
SHELL_CMD_ARG_REGISTER(gcunumeric, NULL, "Displays the selected sensor values as a numeric value",
                       gcunumeric_h, 1, 1);
SHELL_CMD_ARG_REGISTER(gcurec, NULL,
                       "Start and stop writing data to the GCU SD card\r\nNOTE: The file name "
                       "must have a maximum of 5 characters with a three characrter file "
                       "extension. E.g \"abcde.csv\"",
                       gcurec_h, 2, 2);

/* AHU LED cmds */
SHELL_STATIC_SUBCMD_SET_CREATE(led_cmds,
                               SHELL_CMD_ARG(s, NULL,
                                             "set / clear user LEDs with 4 bit bitmask\n"
                                             "usage:\n"
                                             "$ led s <bitmask>\n"
                                             "example: Set LED1 and clear LED2,3,4\n"
                                             "$ led s 1000\n",
                                             led_set, 2, 0),
                               SHELL_CMD_ARG(t, NULL,
                                             "toggle user LEDs with 4 bit bitmask\n"
                                             "usage:\n"
                                             "$ led t <bitmask>\n"
                                             "example: Toggle LED3 and LED4\n"
                                             "$ led t 0011",
                                             led_toggle, 2, 0),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(led, &led_cmds, "Set, clear, and toggle user LEDs", NULL);