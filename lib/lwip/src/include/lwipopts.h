
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* NO_SYS == 1: We use lwIP only with callbacks */
#define NO_SYS					1


/*>>  Memory options ---------- */

/* MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by the C-library . */
#define MEM_LIBC_MALLOC			1

/* MEMP_MEM_INIT==1: Force use of memset to initialize pool memory. */
#define MEMP_MEM_INIT			1

/* MEM_ALIGNMENT: set to 4 byte alignment */
#define MEM_ALIGNMENT			4


/*>> Internal Memory Pool Sizes ---------- */

/* MEMP_NUM_RAW_PCB: Number of raw connection PCBs, not used */
#define MEMP_NUM_RAW_PCB		0          

/* MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections. */
#define MEMP_NUM_TCP_PCB		64

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections. */
#define MEMP_NUM_TCP_PCB_LISTEN	0

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments. */
#define MEMP_NUM_TCP_SEG		255

/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE			512

/*>> ARP options */

/* LWIP_ARP==1: Enable ARP functionality. */
#define LWIP_ARP				0

/*>> DHCP options */

/* LWIP_DHCP==1: Enable DHCP functionality. */
#define LWIP_DHCP				0

/*>> DNS options */
#define LWIP_DNS				1


/*>> UPD options */

/* LWIP_UDP==1: Turn on UDP. */
#define LWIP_UDP				1

/*>> TCP options */

/* LWIP_TCP==1: Turn on TCP. */
#define LWIP_TCP				1

/* TCP Maximum segment size, default value was 1476 */
#define TCP_MSS					1476

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF				(64 * 1024)

/* TCP Window size */
#define TCP_WND					(64 * 1024)

/* TCP Window scaling is enabled */
#define LWIP_WND_SCALE			1

/* TCP Receive scale factor */
#define TCP_RCV_SCALE			1

/* LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT */
#define LWIP_TCP_KEEPALIVE		1

/*>> Pbuf options */

/* PBUF_LINK_HLEN: the number of bytes that should be allocated for a link level header. */
#define PBUF_LINK_HLEN			16

/*>> Sequential layer options */

/* LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c) */
#define LWIP_NETCONN			0

/*>>  Socket options */

/* LWIP_SOCKET==1: Enable Socket API (require to use sockets.c) */
#define LWIP_SOCKET				0

/* Number of simultaneously active timeouts, add 1 for the tunneler, 2 * the number of active forwarders */
#define MEMP_NUM_SYS_TIMEOUT	(LWIP_TCP + IP_REASSEMBLY + PPP_NUM_TIMEOUTS + 2 * MEMP_NUM_TCP_PCB )	


/*>> PPP options (see ppp_opts.h) */

/* PPP_SUPPORT==1: Enable PPP. */
#define PPP_SUPPORT				1
#define PPPOS_SUPPORT			0

#define PPP_IPV4_SUPPORT		1
#define PPP_IPV6_SUPPORT		0
#define VJ_SUPPORT				0


/*>> Statistics options */

/* LWIP_STATS==1: Enable statistics collection in lwip_stats. */
#define LWIP_STATS				1
#define LWIP_STATS_LARGE		1
#define MEM_STATS				0
#define MEMP_STATS				0

/*>> Debug options */
#ifdef _DEBUG
#define LWIP_DEBUG				1
#define PPP_DEBUG				0
#else
#define LWIP_DEBUG				0
#define PPP_DEBUG				0
#endif


#endif /* __LWIPOPTS_H__ */

