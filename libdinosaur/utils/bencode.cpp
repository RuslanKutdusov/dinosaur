/*
 * bencode_v2.cpp
 *
 *  Created on: 23.02.2012
 *      Author: ruslan
 */

#include "bencode.h"

namespace dinosaur {
namespace bencode
{

int decode_str(const char ** data, uint64_t * len, be_str * s)
{
	const char * st = *data;
	char * endptr = NULL;
	uint64_t str_len = strtoll(*data, &endptr, 10);
	if (*endptr != ':')
		return -1;

	*data = ++endptr;
	if (str_len > *len - (*data - st))
		return -1;
	s->len = str_len;
	if (str_len == 0)
	{
		s->ptr = new char[1];
		s->ptr = '\0';
		str_len = 1;
	}
	else
	{
		s->ptr = new char[str_len];
		memcpy(s->ptr, *data, str_len);
		*data += str_len;
	}
	*len -= (*data - st);
	return 0;
}

be_node * decode(const char ** data, uint64_t * len, bool torrent)
{
	if (data == NULL || *data == NULL || len <= 0)
		return NULL;

	const char * correct_keys[]={"announce","announce-list","comment","created by","creation date","encoding","info","length","name","piece length","pieces","private","path","md5sum","files"};
	be_node * ret;
	//std::cout<<"len ="<<*len<<std::endl;
	if (len < 0)
		return NULL;
	switch(**data)
	{
	case 'i':
		{
			//std::cout<<"int\n";
			const char * st = *data;
			++(*data);
			if (**data == 'e')
			{//std::cout<<"bad int1\n";
				return NULL;
			}

			char * endptr;
			long long int val = strtoll(*data, &endptr, 10);
			if (endptr == *data || *endptr != 'e' || ((val == LONG_LONG_MAX || val == LONG_LONG_MIN) && errno == ERANGE))
			{
				//std::cout<<"bad int2\n";
				return NULL;
			}

			*data = ++endptr;
			*len -= (*data - st);
			ret = new be_node;
			ret->type = BE_INT;
			ret->val.i = val;
			//std::cout<<val<<std::endl;
			return ret;
		}
	case '0'...'9':
		{
			//std::cout<<"str\n";
			ret = new be_node;
			memset(ret, 0, sizeof(be_node));
			ret->type = BE_STR;
			if (decode_str(data, len, &ret->val.s) == 0)
			{
				//std::cout<<ret->val.s.ptr<<std::endl;
				return ret;
			}
			else
			{
				//std::cout<<"bad str\n";
				delete ret;
				return NULL;
			}
		}
	case 'l':
		{
			//std::cout<<"list\n";
			*len -= 1;
			++(*data);
			if (**data == 'e')
			{
				//std::cout<<"bad list1\n";
				return NULL;
			}
			ret = new be_node;
			ret->type = BE_LIST;
			ret->val.l.count = 0;
			ret->val.l.elements = NULL;
			while(**data != 'e')
			{
				be_node * element = decode(data, len, torrent);
				if (element == NULL)
				{
					//std::cout<<"bad list2\n";
					_free(ret);
					return NULL;
				}
				be_node * elements = new be_node[++ret->val.l.count];
				if (ret->val.l.count != 1)
				{
					memcpy(elements, ret->val.l.elements, sizeof(be_node)*(ret->val.l.count - 1));
					delete[] ret->val.l.elements;
				}
				memcpy(&elements[ret->val.l.count - 1], element, sizeof(be_node));
				delete element;
				ret->val.l.elements = elements;
			}
			*len -= 1;
			++(*data);
			return ret;
		}
	case 'd':
		{
			//std::cout<<"dict\n";
			*len -= 1;
			++(*data);
			ret = new be_node;
			ret->type = BE_DICT;
			ret->val.d.count = 0;
			ret->val.d.keys = NULL;
			ret->val.d.vals = NULL;
			if (**data == 'e')
			{
				//std::cout<<"bad dict1\n";
				*len -= 1;
				++(*data);
				return ret;
			}
			while(**data != 'e')
			{
				be_str key;
				if (decode_str(data, len, &key) == -1)
				{
					//std::cout<<"bad dict, bad key\n";
					_free(ret);
					return NULL;
				}
				//std::cout<<"key = "<<key.ptr<<std::endl;
				be_node * val = decode(data, len, torrent);
				if (val == NULL)
				{
					delete[] key.ptr;
					//std::cout<<"bad dict, bad val\n";
					_free(ret);
					return NULL;
				}
				if (torrent)
				{
					bool match = false;
					for(int i = 0; i < 15; i++)
					{
						int l = strlen(correct_keys[i]);
						if (key.len == l && strncmp(key.ptr, correct_keys[i], l) == 0)
						{
							match = true;
							break;
						}
					}
					if (!match)
					{
						_free(val);
						delete[] key.ptr;
						continue;
					}
				}
				be_node * vals = new be_node[++ret->val.d.count];
				be_str * keys  = new be_str[ret->val.d.count];
				if (ret->val.d.count != 1)
				{
					memcpy(vals, ret->val.d.vals, sizeof(be_node)*(ret->val.d.count - 1));
					delete[] ret->val.d.vals;
					memcpy(keys, ret->val.d.keys, sizeof(be_str)*(ret->val.d.count - 1));
					delete[] ret->val.d.keys;
				}
				memcpy(&keys[ret->val.d.count - 1], &key, sizeof(be_str));
				memcpy(&vals[ret->val.d.count - 1], val, sizeof(be_node));
				delete val;
				ret->val.d.keys = keys;
				ret->val.d.vals = vals;
			}
			*len -= 1;
			++(*data);
			return ret;
		}
	}
	return NULL;
}

be_node * decode(const char * data, uint64_t len, bool torrent)
{
	uint64_t l = len;
	be_node * t = decode(&data, &l, torrent);
	//std::cout<<"PROCESS = "<<len - l<<std::endl;
	return t;
}

int encode_recursive(be_node * node, char ** buf, uint32_t buf_length, uint32_t * out_len)
{
	if (node == NULL || buf == NULL || *buf == NULL || out_len == NULL)
		return -1;
	//char * ret = NULL;// = (char*)malloc(sizeof(be_node));

	switch (node->type) {
		case BE_STR:
		{
			uint32_t str_length = node->val.s.len;
			char bencode_str_param[22];
			sprintf(bencode_str_param,"%u:",str_length);
			size_t bencode_str_param_len = strlen(bencode_str_param);
			if (buf_length - *out_len < bencode_str_param_len + str_length)
				return -1;
			memcpy(*buf, bencode_str_param, bencode_str_param_len);
			*buf = *buf + bencode_str_param_len;
			memcpy(*buf, node->val.s.ptr, str_length);
			*buf = *buf + str_length;
			*out_len += bencode_str_param_len + str_length;
			break;
		}

		case BE_INT:
		{
			char i[23];
			sprintf(i, "i%llie",node->val.i);
			size_t int_len = strlen(i);
			if (buf_length - *out_len < int_len)
				return -1;
			memcpy(*buf, i, int_len);
			*buf = *buf + int_len;
			*out_len += int_len;
			break;
		}

		case BE_LIST:
		{
			//ret = (char*)malloc(1);
			if (buf_length - *out_len < 1)
				return -1;
			**buf = 'l';
			*buf += 1;
			*out_len += 1;

			for (int i = 0; i < node->val.l.count; i++)
				if (encode_recursive(&node->val.l.elements[i], buf, buf_length, out_len) == -1)
					return -1;

			if (buf_length - *out_len < 1)
				return -1;
			**buf = 'e';
			*buf += 1;
			*out_len += 1;
			break;
		}
		case BE_DICT:
			//ret = (char*)malloc(1);
			if (buf_length - *out_len < 1)
				return -1;
			**buf = 'd';
			*buf += 1;
			*out_len += 1;
			for (int i = 0; i < node->val.d.count; i++) {
				//printf("%s => ", node->val.d[i].key);
				//_dump(node->val.d[i].val, -(indent + 1));

				uint32_t str_length = node->val.d.keys[i].len;
				char bencode_str_param[22];
				sprintf(bencode_str_param,"%u:",str_length);
				size_t bencode_str_param_len = strlen(bencode_str_param);
				if (buf_length - *out_len < bencode_str_param_len + str_length)
						return -1;
				memcpy(*buf, bencode_str_param, bencode_str_param_len);
				*buf = *buf + bencode_str_param_len;
				memcpy(*buf, node->val.d.keys[i].ptr, str_length);
				*buf = *buf + str_length;
				*out_len += bencode_str_param_len + str_length;

				if (encode_recursive(&node->val.d.vals[i], buf, buf_length, out_len) == -1)
					return -1;
			}

			//ret = (char*)realloc(ret,ret_len + 1);
			if (buf_length - *out_len < 1)
				return -1;
			**buf = 'e';
			*buf += 1;
			*out_len += 1;
			break;
	}

	return 0;
}

int encode(be_node * node, char ** buf, uint32_t buf_length, uint32_t * encoded_len)
{
	if (node == NULL || buf == NULL || *buf == NULL || encoded_len == NULL)
			return -1;
	char * changable_ref = *buf;
	*encoded_len = 0;
	return encode_recursive(node, &changable_ref, buf_length, encoded_len);
}

int get_node(be_node * node, const char * key, be_node ** val)
{
	//ret->type = -1;
	if (node == NULL || key == NULL || val == NULL)
		return -1;
	if (node->type == BE_DICT)
	{
		for (int i = 0; i < node->val.d.count; i++)
		{
			int l = strlen(key);
			if (node->val.d.keys[i].len == l && strncmp(node->val.d.keys[i].ptr, key, l) == 0)
			{
				*val = &node->val.d.vals[i];
				return 0;
			}
		}
	}
	return -1;
}

int get_int(be_node * node, const char * key, uint64_t * val)
{
	if (node == NULL || key == NULL || val == NULL)
		return -1;
	if (node->type == BE_DICT)
	{
		for (int i = 0; i < node->val.d.count; i++)
		{
			int l = strlen(key);
			if (node->val.d.keys[i].len == l && strncmp(node->val.d.keys[i].ptr, key, l) == 0 && node->val.d.vals[i].type == BE_INT)
			{
				*val = node->val.d.vals[i].val.i;
				return 0;
			}
		}
	}
	return -1;
}

int get_str(be_node * node, const char * key, be_str ** val)
{
	if (node == NULL || key == NULL || val == NULL)
		return -1;
	if (node->type == BE_DICT)
	{
		for (int i = 0; i < node->val.d.count; i++)
		{
			int l = strlen(key);
			if (node->val.d.keys[i].len == l && strncmp(node->val.d.keys[i].ptr, key, l) == 0 && node->val.d.vals[i].type == BE_STR)
			{
				*val = &node->val.d.vals[i].val.s;
				return 0;
			}
		}
	}
	return -1;
}

int get_list(be_node * node, const char * key, be_list ** val)
{
	if (node == NULL || key == NULL || val == NULL)
		return -1;
	if (node->type == BE_DICT)
	{
		for (int i = 0; i < node->val.d.count; i++)
		{
			int l = strlen(key);
			if (node->val.d.keys[i].len == l && strncmp(node->val.d.keys[i].ptr, key, l) == 0 && node->val.d.vals[i].type == BE_LIST)
			{
				*val = &node->val.d.vals[i].val.l;
				return 0;
			}
		}
	}
	return -1;
}

int get_list_size(be_node * node, const char * key, int * val)
{
	if (node == NULL || key == NULL || val == NULL)
		return -1;
	if (node->type == BE_DICT)
	{
		for (int i = 0; i < node->val.d.count; i++)
		{
			int l = strlen(key);
			if (node->val.d.keys[i].len == l && strncmp(node->val.d.keys[i].ptr, key, l) == 0 && node->val.d.vals[i].type == BE_LIST)
			{
				*val = node->val.d.vals[i].val.l.count;
				return 0;
			}
		}
	}
	return -1;
}

int get_dict(be_node * node, const char * key, be_dict ** val)
{
	if (node == NULL || key == NULL || val == NULL)
		return -1;
	if (node->type == BE_DICT)
	{
		for (int i = 0; i < node->val.d.count; i++)
		{
			int l = strlen(key);
			if (node->val.d.keys[i].len == l && strncmp(node->val.d.keys[i].ptr, key, l) == 0 && node->val.d.vals[i].type == BE_DICT)
			{
				*val = &node->val.d.vals[i].val.d;
				return 0;
			}
		}
	}
	return -1;
}

int get_node(be_node * node, int index, be_node ** val)
{
	//ret->type = -1;
	if (node == NULL || index < 0 || val == NULL)
		return -1;
	if (node->type == BE_LIST)
	{
		if (index < node->val.l.count )
		{
			*val = &node->val.l.elements[index];
			return 0;
		}
	}
	return -1;
}

int get_int(be_node * node, int index, uint64_t * val)
{
	if (node == NULL || index < 0 || val == NULL)
		return -1;
	if (node->type == BE_LIST)
	{
		if (index < node->val.l.count && node->val.l.elements[index].type == BE_INT)
		{
			*val = node->val.l.elements[index].val.i;
			return 0;
		}
	}
	return -1;
}

int get_str(be_node * node, int index, be_str ** val)
{
	if (node == NULL || index < 0 || val == NULL)
		return -1;
	if (node->type == BE_LIST)
	{
		if (index < node->val.l.count && node->val.l.elements[index].type == BE_STR)
		{
			*val = &node->val.l.elements[index].val.s;
			return 0;
		}
	}
	return -1;
}

int get_list(be_node * node, int index, be_list ** val)
{
	if (node == NULL || index < 0 || val == NULL)
		return -1;
	if (node->type == BE_LIST)
	{
		if (index < node->val.l.count && node->val.l.elements[index].type == BE_LIST)
		{
			*val = &node->val.l.elements[index].val.l;
			return 0;
		}
	}
	return -1;
}

int get_list_size(be_node * node, int index, int * val)
{
	if (node == NULL || index < 0 || val == NULL)
		return -1;
	if (node->type == BE_LIST)
	{
		if (index < node->val.l.count && node->val.l.elements[index].type == BE_LIST)
		{
			*val = node->val.l.elements[index].val.l.count;
			return 0;
		}
	}
	return -1;
}

int get_dict(be_node * node, int index, be_dict ** val)
{
	if (node == NULL || index < 0 || val == NULL)
		return -1;
	if (node->type == BE_LIST)
	{
		if (index < node->val.l.count && node->val.l.elements[index].type == BE_DICT)
		{
			*val = &node->val.l.elements[index].val.d;
			return 0;
		}
	}
	return -1;
}

int get_list_size(be_node * node)
{
	if (node == NULL)
		return -1;
	if (node->type == BE_LIST)
		return node->val.l.count;
	return -1;
}

char * str2c_str(be_str * bstr)
{
	uint64_t l = bstr->len;
	char * str = new char[l + 1];
	memset(str, 0, l + 1);
	memcpy(str, bstr->ptr ,l);
	return str;
}

bool is_int(be_node * node)
{
	if (node == NULL)
		return false;
	return node->type == BE_INT;
}

bool is_str(be_node * node)
{
	if (node == NULL)
		return false;
	return node->type == BE_STR;
}

bool is_list(be_node * node)
{
	if (node == NULL)
		return false;
	return node->type == BE_LIST;
}

bool is_dict(be_node * node)
{
	if (node == NULL)
		return false;
	return node->type == BE_DICT;
}

void dump_indent(ssize_t indent)
{
	while (indent-- > 0)
		printf("    ");
}
void _dump(be_node *node, ssize_t indent)
{
	if (node == NULL)
	{
		printf("Nothing to dump, node==null\n");
		return;
	}
	dump_indent(indent);
	indent = abs(indent);

	switch (node->type) {
		case BE_STR:
		{
			uint64_t l = node->val.s.len;
			char * str = new char[l + 1];
			memset(str, 0, l + 1);
			memcpy(str, node->val.s.ptr ,l);
			printf("str = %s (len = %lli)\n", str, l);
			delete[] str;
			break;
		}

		case BE_INT:
			printf("int = %lli\n", node->val.i);
			break;

		case BE_LIST:
			puts("list [");

			for (int i = 0; i < node->val.l.count; i++)
				_dump(&node->val.l.elements[i], indent + 1);

			dump_indent(indent);
			puts("]");
			break;

		case BE_DICT:
			puts("dict {");

			for (int i = 0; i < node->val.d.count; i++)
			{
				dump_indent(indent + 1);
				uint64_t l = node->val.d.keys[i].len;
				char * str = new char[l + 1];
				memset(str, 0, l + 1);
				memcpy(str, node->val.d.keys[i].ptr ,l);
				printf("%s => ", str);
				delete[] str;
				_dump(&node->val.d.vals[i], -(indent + 1));
			}

			dump_indent(indent);
			puts("}");
			break;
	}
}
void dump(be_node *node)
{
	_dump(node, 0);
}

void internal_free(be_node * data)
{
	if (data == NULL)
		return;
	if (data->type == BE_STR)
	{
		delete[] data->val.s.ptr;
	}
	if (data->type == BE_LIST)
	{
		for(int i = 0; i<data->val.l.count; i++)
			internal_free(&data->val.l.elements[i]);
		delete[] data->val.l.elements;
	}
	if (data->type == BE_DICT)
	{
		for(int i = 0; i < data->val.d.count; i++)
		{
			internal_free(&data->val.d.vals[i]);
			delete[] data->val.d.keys[i].ptr;
		}
		delete[] data->val.d.keys;
		delete[] data->val.d.vals;
	}
}

void _free(be_node * data)
{
	internal_free(data);
	delete data;
}

void copy_str(be_str * dst, be_str * src)
{
	dst->len = src->len;
	if (src->len == 0)
		dst->ptr = NULL;
	else
	{
		dst->ptr = new char[src->len];
		memcpy(dst->ptr, src->ptr, src->len);
	}
}

be_node * copy(be_node * src)
{
	if (src == NULL)
		return NULL;
	if (src->type == BE_STR)
	{
		if (src->val.s.len < 0)
			return NULL;
		be_node * node = new be_node;
		node->type = BE_STR;
		copy_str(&node->val.s, &src->val.s);
		/*node->val.s.len = src->val.s.len;
		if (src->val.s.len == 0)
			node->val.s.ptr = NULL;
		else
		{
			node->val.s.ptr = new char[src->val.s.len];
			memcpy(node->val.s.ptr, src->val.s.ptr, src->val.s.len);
		}*/
		return node;
	}
	if (src->type == BE_INT)
	{
		be_node * node = new be_node;
		node->type = BE_INT;
		node->val.i = src->val.i;
		return node;
	}
	if (src->type == BE_LIST)
	{
		if (src->val.l.count < 0)
			return NULL;
		be_node * node = new be_node;
		node->type = BE_LIST;
		node->val.l.count = src->val.l.count;
		if (node->val.l.count == 0)
			node->val.l.elements = NULL;
		else
		{
			node->val.l.elements = new be_node[node->val.l.count];
			for(int i = 0; i< node->val.l.count; i++)
			{
				be_node * list_node =  copy(&src->val.l.elements[i]);
				node->val.l.elements[i].type = list_node->type;
				node->val.l.elements[i].val = list_node->val;
				delete list_node;
			}
		}
		return node;
	}
	if (src->type == BE_DICT)
	{
		if (src->val.d.count < 0)
			return NULL;
		be_node * node = new be_node;
		node->type = BE_DICT;
		node->val.d.count = src->val.d.count;
		if (node->val.d.count == 0)
		{
			node->val.d.keys = NULL;
			node->val.d.vals = NULL;
		}
		else
		{
			node->val.d.keys = new be_str[node->val.d.count];
			node->val.d.vals = new be_node[node->val.d.count];
			for(int i = 0; i < src->val.d.count; i++)
			{
				copy_str(&node->val.d.keys[i], &src->val.d.keys[i]);
				be_node * val =  copy(&src->val.d.vals[i]);
				node->val.d.vals[i].type = val->type;
				node->val.d.vals[i].val = val->val;
				delete val;
			}
		}
		return node;
	}
	return NULL;
}

}
}
