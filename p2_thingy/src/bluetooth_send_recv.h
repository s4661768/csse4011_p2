#ifndef __BLUETOOTH_SEND_RECV_H__
#define __BLUETOOTH_SEND_RECV_H__

#define SEND_DATA_TH_PRIORITY 7
#define SEND_DATA_TH_STACKSIZE KB(1)

#define DEV_ID_TEMP 5
#define DEV_ID_HUM 6
#define DEV_ID_PRES 7
#define DEV_ID_TVOC 8

int send_data_th(void);

#endif