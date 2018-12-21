/**
 * @ingroup     examples
 * @{
 *
 * @file        udp_rx_thr.h
 * @brief       UDP receiver thread header
 *
 * @author      [Jui Po Hung, Mingyu Cui]
 *
 * Github Link: [https://github.com/usc-ee250-fall2018/riot-robert_cmyy]
 *
 * @}
 */

/* Header files are used to share global functions and variables. */

#include "kernel_types.h"

#define UDP_RX_DONE     9 //arbitrary number

typedef struct {
    kernel_pid_t main_pid;
    unsigned int num_pkts;
    uint32_t udp_port;
} udp_rx_args_t;

kernel_pid_t udp_rx_thr_init(void *args);