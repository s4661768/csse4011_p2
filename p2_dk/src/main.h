#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdlib.h>

// GCU Command types
#define POINT 0
#define CIRCLE 1
#define LISSA 2
#define DAC 3
#define GLED 4
#define GPD 5

typedef struct LED_Update {
    uint8_t t; // Flag for toggle. If 1, a toggle is being requested, otherwise a set is being
               // requested.
    char *l;   // Bit mask being sent
} LED_Update;

#endif
