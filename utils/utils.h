/*
 * utils.h
 *
 *  Created on: 02.04.2012
 *      Author: ruslan
 */

#pragma once
#include <stdio.h>
#include <stdint.h>

uint64_t generate_block_id(uint32_t piece, uint32_t block);
uint32_t get_piece_from_id(uint64_t id);
uint32_t get_block_from_id(uint64_t id);
