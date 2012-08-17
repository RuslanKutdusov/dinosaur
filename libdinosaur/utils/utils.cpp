/*
 * utils.cpp
 *
 *  Created on: 02.04.2012
 *      Author: ruslan
 */

#include "utils.h"

namespace dinosaur {

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


void  generate_block_id(uint32_t piece_index, uint32_t block_index, BLOCK_ID & id)
{
	id = BLOCK_ID(piece_index, block_index);
}

uint32_t get_piece_from_block_id(const BLOCK_ID & id)
{
	return id.first;
}

uint32_t get_block_from_block_id(const BLOCK_ID & id)
{
	return id.second;
}

void get_piece_block_from_block_id(const BLOCK_ID & id, uint32_t & piece, uint32_t & block)
{
	piece = id.first;
	block = id.second;
}

}
