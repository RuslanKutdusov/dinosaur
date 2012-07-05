/*
 * Blockcache.h
 *
 *  Created on: 17.04.2012
 *      Author: ruslan
 */

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <queue>
#include <list>
#include <map>
#include <boost/bimap.hpp>
#include "../consts.h"
#include "../err/err_code.h"
#include "../lru_cache/lru_cache.h"

namespace block_cache {

//то что ссылка нормально, всего 4 байт вместо 60, если ставить хэш, ссылкой пользуемся лишь для сравнения
typedef std::pair<void*, uint64_t> cache_key;//(reference to Torrent, piece+block)

struct cache_element
{
	char block[BLOCK_LENGTH];
};

typedef lru_cache::LRU_Cache<cache_key, cache_element> Block_cache;

} /* namespace Bittorrent */
