#ifndef NMAP_H
#define NMAP_H

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <limits.h>
#include <stdbool.h>

typedef struct
{
	u_int32_t source_address;
	u_int32_t destination_address;
	u_int8_t zeros;
	u_int8_t protocol;
	u_int16_t length;
} t_ipv4_pseudo_header;

typedef union
{
	struct sockaddr addr;
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
	struct sockaddr_storage storage;
} t_addr;

typedef struct
{
	int family;
	int protocol;
	t_addr addr;
	socklen_t addrlen;
} t_addrinfo;

typedef enum
{
	ACK = 0,
	SYN,
	FIN,
	NUL,
	XMAS,
	UDP,
} t_technique;

typedef struct
{
	t_technique technique;

	unsigned short port_min;
	unsigned short port_max;
} t_options;

typedef union
{
	struct tcphdr tcp;
	struct udphdr udp;
	struct icmphdr icmp;
} t_packet;

typedef enum
{
	UNSCANNED = 0,

	OPEN = 1 << 0,
	CLOSED = 1 << 1,
	FILTERED = 1 << 2,
	UNFILTERED = 1 << 3,
} t_status;

typedef struct
{
	t_addr interface;
	t_addrinfo destination;
	t_options options;

	int socket;					// used for receiving packets
	int socket_icmp;			// used for receiving icmp packets
	bool timeout;				// true if a timeout occured
	t_status status[USHRT_MAX]; // status of each port
} t_scan;

extern t_scan g_scan;
#endif
