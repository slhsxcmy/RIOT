/**
 * @ingroup     examples
 * @{
 *
 * @file        udp_rx_thr.c
 * @brief       UDP receiver thread
 *
 * @author      [Jui Po Hung, Mingyu Cui]
 *
 * Github Link: [https://github.com/usc-ee250-fall2018/riot-robert_cmyy]
 *
 * @}
 */

#include <inttypes.h>
#include <stdio.h>

#include "thread.h"
#include "msg.h"
#include "net/gnrc.h"
#include "udp_rx_thr.h"

// #define OPENMOTE_BUILD 0
// #if OPENMOTE_BUILD
// 	#include "cc2538_rf.h"
// #else 
	// #define CC2538_RSSI_OFFSET 0
// #endif

// #ifndef CC2538_RSSI_OFFSET
// 	#define CC2538_RSSI_OFFSET          (-73)
// #endif

#define ENABLE_DEBUG (1)
#include "debug.h"

#define PKT_INTERVAL_USEC   (1000000) // main.c in multicaster

#define THREAD_QUEUE_SIZE     (8)
static msg_t thr_msg_queue[THREAD_QUEUE_SIZE];
static char thread_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t thr_pid;

/**
 * TODO (Lab 08): implement this functino for the RSS and PRR lab assignment
 *
 * For each packet you receive, you need to extract the Received Signal Strength
 * Indicator (RSSI) from the layer 2 header. The Received Signal Strength (no I)
 * is equal to the RSSI minus CC2538_RSSI_OFFSET. You must cast the original 
 * RSSI value because it is a 2-complement number. This function should 
 * ultimately print out the RSS of the packet inputted.
 */
static void _print_rss(gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_NETIF);
	gnrc_netif_hdr_t *hdr = (gnrc_netif_hdr_t*) snip->data;
	
	printf("RSS: %d\n",  hdr->rssi - 73);
}

/**
 * TODO (Lab 08): implement this function for the RSS and PRR lab assignment
 *
 * You know the number of packets you need to receive, and how many packets
 * you actually received. Calculate the Packet Reception Ratio and print it
 * out.
 */
static void _print_prr(uint32_t pkt_rcv, uint32_t num_pkts)
{
	printf("PRR: %lu\n", (pkt_rcv*100 / num_pkts));
}

/**
 * TODO: implement this function 
 * 
 * When a packet is received, GNRC will give you the packet as a linked list
 * of snips. Look for the snip of type GNRC_NETTYPE_UNDEF. The data here is
 * the packet's payload. Print the payload. You can assume they are readable
 * ascii characters but you can NOT assume the payload is a null terminated
 * string. Thus, you should print based on the size of the data. FYI, this
 * linked list is not circular. You can us gnrc_pktsnip_search_type().
 */
static void _print_payload(gnrc_pktsnip_t *pkt)
{
	gnrc_pktsnip_t *snip = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_UNDEF);
	printf("%.*s\n", snip->size, (char*)snip->data);
}

static void *_udp_rx_thr(void *arg)
{
	udp_rx_args_t *udp_rx_args = (udp_rx_args_t *)arg; //cast it to the right type to parse
	kernel_pid_t main_pid = udp_rx_args->main_pid;
	int num_pkts = udp_rx_args->num_pkts;
	uint32_t udp_port = udp_rx_args->udp_port;

	msg_t msg, reply; 

	//TODO: what is `msg_t reply` used for? see the documentation in gnrc.h. 
	//This is a weird quirk of the RIOT-OS kernel design, so we have to include it.
	reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
	reply.content.value = -ENOTSUP;
	
	// int rcvd_all_pkts = 0;

	/* TODO: create and init this thread's msg queue for async messages */
	msg_init_queue(thr_msg_queue, sizeof(thr_msg_queue)); 
	
	gnrc_pktsnip_t *pkt = NULL;
	/* TODO: register this thread to UDP packets coming in on udp_port */
	gnrc_netreg_entry_t me_reg;
	me_reg.demux_ctx = udp_port;
	me_reg.target.pid = thread_getpid();

	gnrc_netreg_register(GNRC_NETTYPE_UDP, &me_reg);
	
	bool started = false;
	int pkt_count = 0;
	int pkt_rcv = 0;
	/* TODO: exit the while loop when num_pkts packets have been received */
	while (1) {
		DEBUG("pkt_count: %d pkt_rcv: %d\n", pkt_count, pkt_rcv);
		if(pkt_count == num_pkts){
			break;
		}
		
		if(!started) { //if on first packet, block until the first packet is received
			msg_receive(&msg);
			started = true;
		} else {
			if(xtimer_msg_receive_timeout(&msg, (uint32_t)((float)PKT_INTERVAL_USEC * 1.1)) < 0) {
				/* add more loop control logic here */
				pkt_count++;
				DEBUG("udp_rx_thr: missed packet number %d!\n", pkt_count);
				continue;
			} else { // valid packet
				if(!strncmp("Go Trojans!", gnrc_pktsnip_search_type(msg.content.ptr, GNRC_NETTYPE_UNDEF)->data, 11)){
					pkt_count++;
					pkt_rcv++;
				}
			}
			
		}

		
		/* Use gnrc_pktdump.c as a model and refer to gnrc.h for documentation 
			 on how to structure this thread. If you receive a pointer to a packet
			 make sure to gnrc_pktbuf_release(msg.content.ptr) to free up space!
			 This is not explicitly stated in gnrc.h but you can see how it's
			 done in gnrc_pktdump.c. */
		switch (msg.type) {
			case GNRC_NETAPI_MSG_TYPE_RCV:
					DEBUG("udp_rx_thr: data received: ");
					pkt = msg.content.ptr;
					/* call _print_payload()*/
					_print_payload(pkt);
					/* call _print_rss() */
					_print_rss(pkt);
					break;
			// case GNRC_NETAPI_MSG_TYPE_SND:
			// 		pkt = msg.content.ptr;
			// 		_print_payload(pkt);
			// 		break;
			case GNRC_NETAPI_MSG_TYPE_SET:
			case GNRC_NETAPI_MSG_TYPE_GET:
					msg_reply(&msg, &reply);
					break;
			default:
					DEBUG("udp_rx_thr: received something unexpected");
					break;
		}

		// release memory
		gnrc_pktbuf_release(msg.content.ptr);


	}

	/* call _print_prr() */
	_print_prr(pkt_rcv, num_pkts);

	
	/* TODO: Unregister this thread from UDP packets. Technically unnecessary, 
	but let's do it for completion and good practice. */
	gnrc_netreg_unregister(GNRC_NETTYPE_UDP, &me_reg);

	/* send shutdown message to main thread */
	msg.type = UDP_RX_DONE; //TODO
	msg.content.value = 0; //we are not using this field
	msg_send(&msg, main_pid);

	return 0;
}



kernel_pid_t udp_rx_thr_init(void *args)
{
	/* What is `args` supposed to be?!

		C coders use a pointer to void to tell programmers that this pointer 
		argument can point to anything you need. It could be a pointer to
		a function, variable, struct, etc. This allows for code *flexibility*. 
		When this is called in main(), we cast a pointer to udp_rx_args to (void *) 
		because we know what this pointer actually points to since udp_rx_args_t is 
		a type specific to udp_rx_thr.c/.h. That is, if you are calling 
		udp_rx_thr_init(), you have already read this file which tells shows you
		what will happen to the input argument `args`. 
	*/ 
	
	/* use thread_create() here */
	return thread_create(thread_stack, 
							sizeof(thread_stack), 
							THREAD_PRIORITY_MAIN, 
							THREAD_CREATE_STACKTEST,
							_udp_rx_thr, 
							args, 
							"udp_rx_thr_create");
}
