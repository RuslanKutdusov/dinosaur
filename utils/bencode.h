/*
 * bencode_v2.h
 *
 *  Created on: 23.02.2012
 *      Author: ruslan
 */

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <iostream>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sha1.h"

namespace bencode
{

typedef enum {
	BE_STR,
	BE_INT,
	BE_LIST,
	BE_DICT,
} be_type;

struct be_dict;
struct be_node;

struct be_str
{
	ssize_t len;
	char * ptr;
};

struct be_dict
{
	int count;
	be_str * keys;
	be_node * vals;
};

struct be_list
{
	int count;
	be_node * elements;
};

struct be_node
{
	be_type type;
	union
	{
		be_str s;
		uint64_t i;
		be_list l;
		be_dict d;
	} val;
};

be_node * decode(const char * data, uint64_t len, bool torrent);
int encode(be_node * node, char ** buf, uint32_t buf_length, uint32_t * out_len);
int get_node(be_node * node, const char * key, be_node ** val);
int get_int(be_node * node, const char * key, uint64_t * val);
int get_str(be_node * node, const char * key, be_str ** val);
int get_list(be_node * node, const char * key, be_list ** val);
int get_list_size(be_node * node, const char * key, int * val);
int get_dict(be_node * node, const char * key, be_dict ** val);
int get_node(be_node * node, int index, be_node ** val);
int get_int(be_node * node, int index, uint64_t * val);
int get_str(be_node * node, int index, be_str ** val);
int get_list(be_node * node, int index, be_list ** val);
int get_list_size(be_node * node, int index, int * val);
int get_dict(be_node * node, int index, be_dict ** val);
int get_list_size(be_node * node);
char * str2c_str(be_str * str);
bool is_int(be_node * node);
bool is_str(be_node * node);
bool is_list(be_node * node);
bool is_dict(be_node * node);
void dump(be_node *node);
void _free(be_node * data);

}



