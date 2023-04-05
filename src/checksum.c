#include <string.h>
#include <netdb.h>

#include "types.h"

unsigned short checksum(unsigned short *addr, size_t len)
{
	unsigned long sum = 0;
	for (; len > sizeof(char); len -= sizeof(short))
		sum += *addr++;
	if (len == sizeof(char))
		sum += *(unsigned char *)addr;
	unsigned char bits = sizeof(short) * 8;
	while (sum >> bits)
		sum = (sum & ((1 << bits) - 1)) + (sum >> bits);
	return (~sum);
}

unsigned short tcp_checksum_t_packet(t_packet packet)
{
	t_ipv4_pseudo_header pseudo_header = {
		.source_address = g_data.source_ip.in.s_addr,
		.destination_address = ((struct sockaddr_in *)&g_data.destination.ai_addr)->sin_addr.s_addr,
		.protocol = IPPROTO_TCP,
		.tcp_length = htons(sizeof(t_packet))};

	packet.tcp.check = 0;

	char buffer[sizeof(t_ipv4_pseudo_header) + sizeof(t_packet)];
	memcpy(buffer, &pseudo_header, sizeof(t_ipv4_pseudo_header));
	memcpy(buffer + sizeof(t_ipv4_pseudo_header), &packet, sizeof(t_packet));

	return checksum((unsigned short *)buffer, sizeof(buffer));
}

unsigned short tcp_checksum_tcp(t_packet packet)
{
	t_ipv4_pseudo_header pseudo_header = {
		.source_address = g_data.source_ip.in.s_addr,
		.destination_address = ((struct sockaddr_in *)&g_data.destination.ai_addr)->sin_addr.s_addr,
		.protocol = IPPROTO_TCP,
		.tcp_length = htons(sizeof(struct tcphdr))};

	packet.tcp.check = 0;

	char buffer[sizeof(t_ipv4_pseudo_header) + sizeof(struct tcphdr)];
	memcpy(buffer, &pseudo_header, sizeof(t_ipv4_pseudo_header));
	memcpy(buffer + sizeof(t_ipv4_pseudo_header), &packet, sizeof(struct tcphdr));

	return checksum((unsigned short *)buffer, sizeof(buffer));
}
