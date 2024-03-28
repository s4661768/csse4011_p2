/*
 * Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <zephyr/kernel.h> // MY Addition
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "bluetooth_send_recv.h"

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

// Print statements
// #define HUMIDITY_PRINTS
// #define TEMPERATURE_PRINTS
// #define PRESSURE_PRINTS
// #define TVOC_PRINTS

// Thread Macros
#define LPS22_TH_STACKSIZE KB(1)
#define LPS22_TH_PRIORITY 6

#define HTS221_TH_STACKSIZE KB(1)
#define HTS221_TH_PRIORITY 6

#define BLUETOOTH_SEND_TH_STACKSIZE KB(1)
#define BLUETOOTH_SEND_TH_PRIORITY 5

#define CCS811_TH_STACKSIZE KB(1)
#define CCS811_TH_PRIORITY 6

// Offset Macros
#define PRES_OFFSET 14
#define TEMP_OFFSET 2
#define HUMIDITY_OFFSET 20
#define TVOC_OFFSET 0

LOG_MODULE_REGISTER(Thingy_52);

/* Fifo queue Initialisation */
struct k_fifo bt_q;

K_MSGQ_DEFINE(bt_msgq, 4, 10, 4);

/*
 * Set iBeacon demo advertisement data. These values are for
 * demonstration only and must be changed for production environments!
 *
 * UUID:  18ee1516-016b-4bec-ad96-bcb96d166e97
 * UUID:  18ee1516-016b-4bec-ad96-bcb96d166e47
 * Major: 0
 * Minor: 0
 * RSSI:  -56 dBm
 */
static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
                  0x02, 0x15,                            /* iBeacon */
                  0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
                  0x01, 0x6b,                            /* UUID[11..10] */
                  0x4b, 0xec,                            /* UUID[9..8] */
                  0xad, 0x96,                            /* UUID[7..6] */
                  0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x47,    /* UUID[5..0] */
                  0xAA, 0xCC,                            /* Major */
                  0xAA, 0xCC,                            /* Minor */
                  IBEACON_RSSI)                          /* Calibrated RSSI @ 1m */
};

static void bt_ready(int err) {
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    /* Start advertising */
    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("iBeacon started\n");
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++ //
/* 						SENSOR CODE */
// +++++++++++++++++++++++++++++++++++++++++++++++++++++ //

static int16_t lps22_process_sample(const struct device *dev) {
    static unsigned int obs;
    struct sensor_value pressure, temp;

    if (sensor_sample_fetch(dev) < 0) {
        printf("Sensor sample update error\n");
        return 0;
    }

    if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
        printf("Cannot read LPS22HB pressure channel\n");
        return 0;
    }

    if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
        printf("Cannot read LPS22HB temperature channel\n");
        return 0;
    }

    ++obs;

    int16_t temp_num = sensor_value_to_double(&pressure) * 100;

#ifdef PRESSURE_PRINTS
    LOG_INF("==== LPS22 Sensor ====\r\n");
    printf("Observation:%u\n", obs);
    LOG_INF("Observation:%u\n", obs);

    /* display pressure */
    printf("Pressure:%.1f kPa\n", sensor_value_to_double(&pressure));
    LOG_INF("Pressure:%.1f kPa\n", sensor_value_to_double(&pressure));
    LOG_INF("Pressure (int):%d kPa\n", temp_num);
#endif
    return temp_num + PRES_OFFSET;
}

static bool app_fw_2;

#ifdef TVOC_PRINTS
static const char *now_str(void) {
    static char buf[16]; /* ...HH:MM:SS.MMM */
    uint32_t now = k_uptime_get_32();
    unsigned int ms = now % MSEC_PER_SEC;
    unsigned int s;
    unsigned int min;
    unsigned int h;

    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
    return buf;
}
#endif

static int do_fetch(const struct device *dev) {
    struct sensor_value tvoc; // co2, voltage, current;
    int rc = 0;
    // int baseline = -1;

#ifdef CONFIG_APP_MONITOR_BASELINE
    rc = ccs811_baseline_fetch(dev);
    if (rc >= 0) {
        baseline = rc;
        rc = 0;
    }
#endif
    if (rc == 0) {
        rc = sensor_sample_fetch(dev);
    }
    if (rc == 0) {
        const struct ccs811_result_type *rp = ccs811_result(dev);

        sensor_channel_get(dev, SENSOR_CHAN_VOC, &tvoc);
        int16_t temp_num = ((tvoc.val1 + TVOC_OFFSET) % 1188) * 10;
#ifdef TVOC_PRINTS
        LOG_INF("==== CCS811 Sensor ====\r\n");
        printk("\n[%s]: CCS811: %u ppm eCO2; %u ppb eTVOC\n", now_str(), co2.val1, tvoc.val1);

        LOG_INF("\n[%s]: %d ppb eTVOC\n", now_str(), temp_num);
#endif

        uint8_t data[4];
        // LOG_INF("Recv Pressure: %d kPa\n", temp_num);

        data[0] = DEV_ID_TVOC;
        data[1] = 0x00;
        data[2] = (temp_num >> 8) & 0xFF;
        data[3] = (temp_num) & 0xFF;

        // k_fifo_put(&bt_q, data);
        if (k_msgq_put(&bt_msgq, &data, K_MSEC(10)) != 0) {
            LOG_INF("Couldn't put pressure val in queue\r\n");
        }
        k_sleep(K_MSEC(300));
        if (app_fw_2 && !(rp->status & CCS811_STATUS_DATA_READY)) {
            printk("STALE DATA\n");
        }

        if (rp->status & CCS811_STATUS_ERROR) {
            printk("ERROR: %02x\n", rp->error);
        }
    }
    return rc;
}

#ifndef CONFIG_CCS811_TRIGGER_NONE

static void trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {
    int rc = do_fetch(dev);

    if (rc == 0) {
        printk("Triggered fetch got %d\n", rc);
    } else if (-EAGAIN == rc) {
        printk("Triggered fetch got stale data\n");
    } else {
        printk("Triggered fetch failed: %d\n", rc);
    }
}

#endif /* !CONFIG_CCS811_TRIGGER_NONE */

static void do_main(const struct device *dev) {
    while (true) {
        int rc = do_fetch(dev);

        if (rc == 0) {
            printk("Timed fetch got %d\n", rc);
        } else if (-EAGAIN == rc) {
            printk("Timed fetch got stale data\n");
        } else {
            printk("Timed fetch failed: %d\n", rc);
            break;
        }
        k_msleep(300);
    }
}

uint32_t hts221_process_sample(const struct device *dev) {
    static unsigned int obs;
    struct sensor_value temp, hum;
    if (sensor_sample_fetch(dev) < 0) {
        printf("Sensor sample update error\n");
        return 0;
    }

    if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
        printf("Cannot read HTS221 temperature channel\n");
        return 0;
    }

    if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
        printf("Cannot read HTS221 humidity channel\n");
        return 0;
    }

    ++obs;
#ifdef HUMIDITY_PRINTS
    LOG_INF("==== HTS221 humidity channel ====\r\n");
    printf("Observation:%u\n", obs);
    LOG_INF("Observation:%u\n", obs);

    /* display temperature */
    printf("Temperature:%.1f C\n", sensor_value_to_double(&temp));
    LOG_INF("Temperature:%.1f C\n", sensor_value_to_double(&temp));

    /* display humidity */
    printf("Relative Humidity:%.1f%%\n", sensor_value_to_double(&hum));

    LOG_INF("Relative Humidity:%.1f%%\n", sensor_value_to_double(&hum));
#endif

    int16_t temp_temp = (sensor_value_to_double(&temp) + TEMP_OFFSET) * 100;

    int16_t temp_hum = (sensor_value_to_double(&hum) + HUMIDITY_OFFSET) * 100;
    if (temp_hum > 100 * 100) { // Ceiling  the humidity value
        temp_hum = 100 * 100;
    }

    uint32_t retval = (temp_temp << 16) | (temp_hum);

    return retval;
}

int bluetooth_send_th(void) {

    /* BLUETOOTH */
    int err;

    printk("Starting iBeacon Demo\n");

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
    }

    while (1) {

        uint8_t data_recv[4];

        // LOG_INF("Attempting queue read\r\n");
        if (k_msgq_get(&bt_msgq, &data_recv, K_MSEC(10)) == 0) {
            // LOG_INF("BT_TH: Received from msgq");
            LOG_INF("data: %d, %d, %d, %d\r\n", data_recv[0], data_recv[1], data_recv[2],
                    data_recv[3]);

            const struct bt_data ad[] = {
                BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
                BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x4c, 0x00, /* Apple */
                              0x02, 0x15,                            /* iBeacon */
                              0x18, 0xee, 0x15, 0x16,                /* UUID[15..12] */
                              0x01, 0x6b,                            /* UUID[11..10] */
                              0x4b, 0xec,                            /* UUID[9..8] */
                              0xad, 0x96,                            /* UUID[7..6] */
                              0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x47,    /* UUID[5..0] */
                              data_recv[0], data_recv[1],            /* Major */
                              data_recv[2], data_recv[3],            /* Minor */
                              IBEACON_RSSI)                          /* Calibrated RSSI @ 1m */
            };

            int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                LOG_ERR("Bluetooth init failed (err %d)\n", err);
            }
        }
        k_sleep(K_MSEC(100));
    }

    return 0;
}

/* LPS22 */
int lps22_th(void) {

    /* LPS22 */
    const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hb_press);

    if (!device_is_ready(dev)) {
        printf("Device %s is not ready\n", dev->name);
        return 0;
    }

    int16_t pressure = 0;
    uint8_t data[4];

    while (true) {
        pressure = lps22_process_sample(dev);

        data[0] = DEV_ID_PRES;
        data[1] = 0x00;
        data[2] = (pressure >> 8) & 0xFF;
        data[3] = pressure & 0xFF;

        if (k_msgq_put(&bt_msgq, &data, K_MSEC(10)) != 0) {
            LOG_INF("Couldn't put pressure val in queue\r\n");
        }
        k_sleep(K_MSEC(1000));
    }

    return 0;
}

/* HTS221 */
int hts221_th(void) {

    /* HTS221 */
    const struct device *const dev = DEVICE_DT_GET_ONE(st_hts221);

    if (!device_is_ready(dev)) {
        printf("Device %s is not ready\n", dev->name);
        return 0;
    }

    uint32_t retval = 0;
    int16_t temperature = 0;
    int16_t humidity = 0;
    uint8_t data[4];
    while (true) {
        retval = hts221_process_sample(dev);
        temperature = (retval >> 16) & 0xFFFF;
        humidity = retval & 0xFFFF;

        data[0] = DEV_ID_TEMP;
        data[1] = 0x00;
        data[2] = (temperature >> 8) & 0xFF;
        data[3] = temperature & 0xFF;

        if (k_msgq_put(&bt_msgq, &data, K_MSEC(10)) != 0) {
            LOG_INF("Couldn't put temperature val in queue\r\n");
        }

        k_sleep(K_MSEC(800));

        data[0] = DEV_ID_HUM;
        data[1] = 0x00;
        data[2] = (humidity >> 8) & 0xFF;
        data[3] = humidity & 0xFF;

        if (k_msgq_put(&bt_msgq, &data, K_MSEC(10)) != 0) {
            LOG_INF("Couldn't put humidity val in queue\r\n");
        }

        k_sleep(K_MSEC(800));
    }

    return 0;
}

/* CCS811 */
int ccs811_th(void) {
    const struct device *const dev = DEVICE_DT_GET_ONE(ams_ccs811);
    struct ccs811_configver_type cfgver;
    int rc;

    if (!device_is_ready(dev)) {
        printk("Device %s is not ready\n", dev->name);
        return 0;
    }

    printk("device is %p, name is %s\n", dev, dev->name);

    rc = ccs811_configver_fetch(dev, &cfgver);
    if (rc == 0) {
        printk("HW %02x; FW Boot %04x App %04x ; mode %02x\n", cfgver.hw_version,
               cfgver.fw_boot_version, cfgver.fw_app_version, cfgver.mode);
        app_fw_2 = (cfgver.fw_app_version >> 8) > 0x11;
    }

#ifdef CONFIG_APP_USE_ENVDATA
    struct sensor_value temp = {CONFIG_APP_ENV_TEMPERATURE};
    struct sensor_value humidity = {CONFIG_APP_ENV_HUMIDITY};

    rc = ccs811_envdata_update(dev, &temp, &humidity);
    printk("ENV_DATA set for %d Cel, %d %%RH got %d\n", temp.val1, humidity.val1, rc);
#endif

#ifdef CONFIG_CCS811_TRIGGER
    struct sensor_trigger trig = {0};

#ifdef CONFIG_APP_TRIGGER_ON_THRESHOLD
    printk("Triggering on threshold:\n");
    if (rc == 0) {
        struct sensor_value thr = {
            .val1 = CONFIG_APP_CO2_MEDIUM_PPM,
        };
        rc = sensor_attr_set(dev, SENSOR_CHAN_CO2, SENSOR_ATTR_LOWER_THRESH, &thr);
        printk("L/M threshold to %d got %d\n", thr.val1, rc);
    }
    if (rc == 0) {
        struct sensor_value thr = {
            .val1 = CONFIG_APP_CO2_HIGH_PPM,
        };
        rc = sensor_attr_set(dev, SENSOR_CHAN_CO2, SENSOR_ATTR_UPPER_THRESH, &thr);
        printk("M/H threshold to %d got %d\n", thr.val1, rc);
    }
    trig.type = SENSOR_TRIG_THRESHOLD;
    trig.chan = SENSOR_CHAN_CO2;
#elif defined(CONFIG_APP_TRIGGER_ON_DATAREADY)
    printk("Triggering on data ready\n");
    trig.type = SENSOR_TRIG_DATA_READY;
    trig.chan = SENSOR_CHAN_ALL;
#else
#error Unhandled trigger on
#endif
    if (rc == 0) {
        rc = sensor_trigger_set(dev, &trig, trigger_handler);
    }
    if (rc == 0) {
#ifdef CONFIG_APP_TRIGGER_ON_DATAREADY
        while (true) {
            k_sleep(K_FOREVER);
        }
#endif
    }
    printk("Trigger installation got: %d\n", rc);
#endif /* CONFIG_CCS811_TRIGGER */
    if (rc == 0) {
        do_main(dev);
    }
    return 0;
}

K_THREAD_DEFINE(lps22_th_id, LPS22_TH_STACKSIZE, lps22_th, NULL, NULL, NULL, LPS22_TH_PRIORITY, 0,
                0);

K_THREAD_DEFINE(hts221_th_id, HTS221_TH_STACKSIZE, hts221_th, NULL, NULL, NULL, HTS221_TH_PRIORITY,
                0, 0);

K_THREAD_DEFINE(ccs811_th_id, CCS811_TH_STACKSIZE, ccs811_th, NULL, NULL, NULL, CCS811_TH_PRIORITY,
                0, 0);

K_THREAD_DEFINE(bluetooth_send_th_id, BLUETOOTH_SEND_TH_STACKSIZE, bluetooth_send_th, NULL, NULL,
                NULL, BLUETOOTH_SEND_TH_PRIORITY, 0, 0);