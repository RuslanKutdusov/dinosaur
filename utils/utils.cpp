/*
 * utils.cpp
 *
 *  Created on: 02.04.2012
 *      Author: ruslan
 */

#include "utils.h"

uint64_t generate_block_id(uint32_t piece, uint32_t block)
{
	return ((uint64_t)piece << 32) + block;
}

uint32_t get_piece_from_id(uint64_t id)
{
	uint32_t block_index = get_block_from_id(id);
	return (id - block_index)>>32;
}

uint32_t get_block_from_id(uint64_t id)
{
	return (id & (uint32_t)4294967295);
}


