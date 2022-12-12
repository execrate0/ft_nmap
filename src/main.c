#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/ip.h>

#include "nmap.h"
#include "functions.h"

t_data g_data = {0};

void result(char *answer)
{
	printf("\n");
	printf("Scan results:\n");
	for (int i = 0; i < 1024; i++)
	{
		if (answer[i] == OPEN)
			printf("%d: OPEN\n", i);
		else if (answer[i] == CLOSED)
			printf("%d: CLOSED\n", i);
		else if (answer[i] == UNEXPECTED)
			printf("%d: UNEXPECTED\n", i);
	}
}

void check_packet(char *answer, int save_port, int idx)
{
	while (1)
	{
		char buffer[BUFFER_SIZE] = {0};
		int ret = 0;

		ret = recvfrom(g_data.sock, buffer, 4096, 0, NULL, NULL);
		if (ret < 0)
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else
				error(1, "recvfrom: %s\n", strerror(errno));
		else if (ret == 0)
			break;

		struct iphdr *ip = (struct iphdr*)buffer;
		struct tcphdr *tcp = (struct tcphdr*)(buffer + (ip->ihl * 4));

		if (ntohs(tcp->dest) != ntohs(save_port))
			continue;
		if (tcp->syn && tcp->ack)
			answer[idx] = OPEN;
		else if (tcp->rst)
			answer[idx] = CLOSED;
		else
			answer[idx] = UNEXPECTED;
	}
}

void send_packet()
{
	srand(time(0));
	char answer[1026] = {0};
	short save_port = -1;

	struct pseudo_header psh = {0};

	// TCP pseudo header for checksum calculation
	psh.source_address = g_data.host->sin_addr.s_addr;
	psh.dest_address = ((struct sockaddr_in *)(g_data.res->ai_addr))->sin_addr.s_addr;
	psh.protocol = IPPROTO_TCP;
	psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);
	// fill pseudo packet
	char pseudogram[sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE] = {0};
	memcpy(pseudogram, (char*)&psh, sizeof(struct pseudo_header));

	for (int port = 1; port < 1025; port++)
	{
		char datagram[BUFFER_SIZE] = {0};
		struct tcphdr *tcp_to = (struct tcphdr*)(datagram);
		
		printf("Scanning port %d\r", port);
		// TCP header configuration
		save_port = tcp_to->source = htons(rand() % (65000 - 100 + 1) + 100);
		tcp_to->dest = htons(port);
		tcp_to->seq = htonl(0);
		tcp_to->doff = 10;
		tcp_to->syn = 1;
		tcp_to->window = htons(5840);

		// TCP pseudo header for checksum calculation
		memcpy(pseudogram + sizeof(struct pseudo_header), tcp_to, sizeof(struct tcphdr) + OPT_SIZE);

		// TCP options are only set in the SYN packet
		// ---- set mss ----
		datagram[20] = 0x02;
		datagram[21] = 0x04;
		int16_t mss = htons(48); // mss value
		memcpy(datagram + 22, &mss, sizeof(int16_t));
		// ---- enable SACK ----
		datagram[24] = 0x04;
		datagram[25] = 0x02;
		// do the same for the pseudo header
		pseudogram[32] = 0x02;
		pseudogram[33] = 0x04;
		memcpy(pseudogram + 34, &mss, sizeof(int16_t));
		pseudogram[36] = 0x04;
		pseudogram[37] = 0x02;

		tcp_to->check = checksum((unsigned short*)pseudogram, sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE);

		// printf("\nSending SYN packet to %s:%d\n\n", inet_ntoa(*(struct in_addr *)&((struct sockaddr_in *)res->ai_addr)->sin_addr), ntohs(tcp_to->dest));
		if (sendto(g_data.sock, datagram, sizeof(struct tcphdr) + OPT_SIZE, 0, g_data.res->ai_addr, g_data.res->ai_addrlen) < 0)
			error(1, "sendto: %s\n", strerror(errno));

		check_packet(answer, save_port, port);
	}
	result(answer);
	freeaddrinfo(g_data.res);
}

int main(int ac, char **av)
{
	if (getuid() != 0)
		error(1, "usage: You need to be root to run this program\n");

	if (!av[1] || ac != 2)
		error(1, "usage: %s <host>", av[0]);

	g_data.res = get_addr(av[1]);
	g_data.host = get_ifaddr();

	// char ip[INET6_ADDRSTRLEN];
	// for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
	// 	if (p->ai_family == AF_INET6)
    //     	inet_ntop(p->ai_family, &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr, ip, sizeof(ip));
	// 	else
	// 		inet_ntop(p->ai_family, &((struct sockaddr_in *)p->ai_addr)->sin_addr, ip, sizeof(ip));
    //     printf("%s with IPV%d: %s\n", av[1], p->ai_family == AF_INET ? 4 : 6, ip);
    // }

	create_socket();
	send_packet();
	return 0;
}
