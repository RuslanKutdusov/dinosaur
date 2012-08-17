/*
 * torrent_types.h
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#ifndef TORRENT_TYPES_H_
#define TORRENT_TYPES_H_

#include "../consts.h"
#include "../fs/fs.h"

namespace dinosaur {
namespace torrent
{

enum PIECE_STATE
{
	PIECE_STATE_NOT_FIN,
	PIECE_STATE_FIN_HASH_OK,
	PIECE_STATE_FIN_HASH_BAD
};

enum PEER_ADD
{
	PEER_ADD_TRACKER,
	PEER_ADD_INCOMING
};

}
}


#endif /* TORRENT_TYPES_H_ */
