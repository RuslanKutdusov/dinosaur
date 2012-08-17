/*
 * utils.h
 *
 *  Created on: 02.04.2012
 *      Author: ruslan
 */

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <utility>

namespace dinosaur {

typedef std::pair<uint32_t, uint32_t> BLOCK_ID;

char *read_file(const char *file, uint64_t *len, uint64_t max_len);

void  generate_block_id(uint32_t piece_index, uint32_t block_index, BLOCK_ID & id);
uint32_t get_piece_from_block_id(const BLOCK_ID & id);
uint32_t get_block_from_block_id(const BLOCK_ID & id);
void get_piece_block_from_block_id(const BLOCK_ID & id, uint32_t & piece, uint32_t & block);

}
