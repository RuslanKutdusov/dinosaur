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

void get_piece_block_from_id(uint64_t id, uint32_t & piece, uint32_t & block)
{
	block = get_block_from_id(id);
	piece = (id - block)>>32;
}

char *read_file(const char *file, uint64_t *len, uint64_t max_len)
{
	struct stat st;
	char *ret = NULL;
	FILE *fp;

	if (stat(file, &st))
		return ret;
	*len = st.st_size;

	if (*len > max_len)
		return NULL;

	fp = fopen(file, "r");
	if (!fp)
		return ret;

	ret = (char*)malloc(*len);
	if (!ret)
		return ret;

	fread(ret, 1, *len, fp);

	fclose(fp);

	return ret;
}
