/*
 * torrent_utils.cpp
 *
 *  Created on: 17.07.2012
 *      Author: ruslan
 */


#include "torrent.h"

namespace torrent
{

void set_bitfield(uint32_t piece, uint32_t piece_count, BITFIELD bitfield)
{
	if (piece >= piece_count)
		return;
	int byte_index = (int)floor(piece / 8);
	int bit_index = piece - byte_index * 8;
	int bit = (int)pow(2.0f, 7 - bit_index);
	bitfield[byte_index] |= bit;
}

void reset_bitfield(uint32_t piece, uint32_t piece_count, BITFIELD bitfield)
{
	if (piece >= piece_count)
		return;
	int byte_index = (int)floor(piece / 8);
	int bit_index = piece - byte_index * 8;
	int bit = (int)pow(2.0f, 7 - bit_index);
	unsigned char mask;
	memset(&mask, 255, 1);
	mask ^= bit;
	bitfield[byte_index] &= mask;
}

bool bit_in_bitfield(uint32_t piece, uint32_t piece_count, BITFIELD bitfield)
{
	if (piece >= piece_count)
			return false;
	int byte_index = (int)floor(piece / 8);
	int bit_index = piece - byte_index * 8;
	int bit = (int)pow(2.0f, 7 - bit_index);
	return (bitfield[byte_index] & bit) > 0;
}

void get_peer_key(sockaddr_in * addr, std::string & key)
{
	key = inet_ntoa(addr->sin_addr);
	uint16_t port = ntohs(addr->sin_port);
	char port_c[256];
	sprintf(port_c, "%u", port);
	key.append(":");
	key.append(port_c);
}

}

