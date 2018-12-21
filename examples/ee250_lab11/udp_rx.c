/**
 * @ingroup     examples
 * @{
 *
 * @file        udp_rx.c
 * @brief       UDP receiver thread
 *
 * @author      Mingyu Cui
 *              Jui Po Hung
 * Github Link: https://github.com/usc-ee250-fall2018/finalproj-riot-cmyyyyyyy
 *
 * @}
 */

#include <inttypes.h>
#include <stdio.h>

#include "thread.h"
#include "msg.h"
#include "net/gnrc.h"
#include "udp_rx.h"
#include "timex.h"
#include "mutex.h"
#include "random.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

//TODO
#define PRNG_INITVAL            11  //TODO: please use a unique int
#define SLEEP_MSG_STR           "sleep"
#define SLEEP_MSG_LEN           5

#define SLEEP_INTERVAL_SECS     (4)
#define RAND_USLEEP_MIN         (0)
#define RAND_USLEEP_MAX         (1000000)

#define THREAD_QUEUE_SIZE     (8)
static msg_t thr_msg_queue[THREAD_QUEUE_SIZE];
static char thread_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t thr_pid;


static void *_udp_rx(void *arg)
{
	udp_rx_args_t *udp_rx_args = (udp_rx_args_t *)arg; 
	
	mutex_t *mutex_ptr = udp_rx_args->mutex;
	uint32_t udp_port = udp_rx_args->udp_port;
	
	msg_t msg, reply; 

	//TODO: what is `msg_t reply` used for? see the documentation in gnrc.h. 
	//This is a weird quirk of the RIOT-OS kernel design, so we have to include it.
	reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
	reply.content.value = -ENOTSUP;

	/* TODO: create and init this thread's msg queue for async messages */
	msg_init_queue(thr_msg_queue, sizeof(thr_msg_queue)); 

	/* TODO: register this thread to UDP packets coming in on udp_port */
	gnrc_netreg_entry_t me_reg = {
		.demux_ctx = udp_port,
		.target.pid = thread_getpid()
	};
	gnrc_netreg_register(GNRC_NETTYPE_UDP, &me_reg);
	

	/* initialize PRNG */
	random_init(PRNG_INITVAL);
	printf("PRNG initialized to current time: %d\n", PRNG_INITVAL);

	while (1) {
		msg_receive(&msg);
		mutex_lock(mutex_ptr);

		switch (msg.type) {
			case GNRC_NETAPI_MSG_TYPE_RCV:
				DEBUG("udp_rx: data received\n");
				// TODO: if(msg size is valid and msg includes sleep string){
				if(!strncmp(SLEEP_MSG_STR, gnrc_pktsnip_search_type(msg.content.ptr, GNRC_NETTYPE_UNDEF)->data, SLEEP_MSG_LEN)){
					DEBUG("sleeping for %d seconds\n", SLEEP_INTERVAL_SECS);
				
					/* additional random sleep to reduce network collisions */
					uint32_t interval = random_uint32_range(RAND_USLEEP_MIN, RAND_USLEEP_MAX);
					DEBUG("plus a random sleep: %d microseconds\n", (int)interval);
					
					xtimer_sleep(SLEEP_INTERVAL_SECS);
					xtimer_usleep(interval);
				}
				gnrc_pktbuf_release(msg.content.ptr);
				break;

			//TODO
			case GNRC_NETAPI_MSG_TYPE_SET:
			case GNRC_NETAPI_MSG_TYPE_GET:
				msg_reply(&msg, &reply);
				break;	

			default:
				DEBUG("udp_rx_thr: received something unexpected");
				break;
		}
		mutex_unlock(mutex_ptr);
	}

	/* should never be reached */
	DEBUG("ERROR!\n");
	return 0;
}

kernel_pid_t udp_rx_init(void *args)
{
	// return udp_rx_pid;
	return thread_create(thread_stack, 
							sizeof(thread_stack), 
							THREAD_PRIORITY_MAIN, 
							THREAD_CREATE_STACKTEST,
							_udp_rx, 
							args, 
							"udp_rx_thr_create");
}