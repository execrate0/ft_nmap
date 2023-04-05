#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/ip.h>

#include "functions.h"

void create_packet_xmas()
{
	// g_data.packet.tcp.source = htons(rand() % USHRT_MAX);
	g_data.packet.tcp.source = htons(4242);
	g_data.packet.tcp.doff = 5;
	g_data.packet.tcp.fin = 1;
	g_data.packet.tcp.psh = 1;
	g_data.packet.tcp.urg = 1;
	g_data.packet.tcp.window = htons(1024);
}

void create_packet_fin()
{
	// g_data.packet.tcp.source = htons(rand() % USHRT_MAX);
	g_data.packet.tcp.source = htons(4242);
	g_data.packet.tcp.doff = 5;
	g_data.packet.tcp.fin = 1;
	g_data.packet.tcp.window = htons(1024);
}

void create_packet_null()
{
	// g_data.packet.tcp.source = htons(rand() % USHRT_MAX);
	g_data.packet.tcp.source = htons(4242);
	g_data.packet.tcp.doff = 5;
	g_data.packet.tcp.window = htons(1024);
}

void send_packet_others(unsigned short port)
{
	g_data.packet.tcp.dest = htons(port);
	g_data.packet.tcp.check = tcp_checksum(g_data.packet, 0);

	if (sendto(g_data.socket, &g_data.packet, sizeof(struct tcphdr), 0, &g_data.destination.ai_addr, g_data.destination.ai_addrlen) < 0)
		error(1, "sendto: %s\n", strerror(errno));
}

void receive_packet_others(unsigned short port)
{
	unsigned short source_port = ntohs(g_data.packet.tcp.source);
	unsigned short destination_port = port;

	size_t packet_size = sizeof(struct iphdr) + sizeof(struct tcphdr);
	char packet_buffer[packet_size];
	while (1)
	{
		if ((recvfrom(g_data.socket, packet_buffer, packet_size, 0, NULL, NULL)) < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				g_data.result[destination_port] = OPEN_FILTERED;
			else
				error(1, "recvfrom: %s\n", strerror(errno));
			break;
		}

		struct iphdr *ip_header = (struct iphdr *)packet_buffer;
		if (ip_header->protocol != IPPROTO_TCP)
			continue;

		struct tcphdr *packet = (struct tcphdr *)(packet_buffer + sizeof(struct iphdr));
		if (ntohs(packet->source) != destination_port)
			continue;
		if (ntohs(packet->dest) != source_port)
			continue;

		if (packet->rst)
			g_data.result[destination_port] = CLOSED;
		else
			g_data.result[destination_port] = UNEXPECTED;
		break;
	}
}