#ifndef __SHELL_COMMANDS_H__
#define __SHELL_COMMANDS_H__

#include <stdlib.h>

/* Help messages for GCU shell commands */
#define DAC_CMD_HELP_MSG                                                                           \
    "Set the voltage values on the X and Y DACs Voltages must be integers in range [0, "           \
    "33].\r\nExample: Setting voltages to 1.2v and 3v\r\ndac 12 30\r\nNOTED: Any non=integer "     \
    "argument will be interpretted as 0v\r\n"
#define LJ_CMD_HELP_MSG                                                                            \
    "Set that parameters of the sinusoids of the lissajour Curves. Set the amplitude, phase "      \
    "shift, and period of either the x or y sign wave in the following format:\r\n ls 'x'/'y' a "  \
    "b t\r\na: the desired amplitude in decivolts\r\nb: Multiples of pi between 0 and 2. To send " \
    "1.2 enter 12\r\nt: Period in milliseconds.\r\n\nExample: Amplitude of 2.3 volts, phase "      \
    "shift of 1.2*pi, period of 1.5sec\r\nlj 23 12 1500\r\n"

/**
 * Callback for the AHU 'led' shell command.
 */
int led_set(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the AHU 'led' shell command.
 */
int led_toggle(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'time' shell command.
 */
int system_timer_read(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'point' shell command.
 */
int point_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'move_c' shell command.
 */
int move_c_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'circle' shell command.
 */
int circle_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'lj' shell command.
 */
int lissajous_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'dac' shell command.
 */
int dac_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'gled' shell command.
 */
int gled_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'gpd' shell command.
 */
int gpd_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'gcugraph' shell command.
 */
int gcugraph_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'gcumeter' shell command.
 */
int gcumeter_h(const struct shell *shell, size_t argc, char *argv[]);

/**
 * Callback for the 'gcugraph' shell command.
 */
int gcurec_h(const struct shell *shell, size_t argc, char *argv[]);
#endif