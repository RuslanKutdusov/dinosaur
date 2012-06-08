/*
 * consts.h
 *
 *  Created on: 17.02.2012
 *      Author: ruslan
 */

#ifndef CONSTS_H_
#define CONSTS_H_

#define MAX_FILENAME_LENGTH 2048
#define STATE_FILE_VERSION 1
#define CONFIG_FILE_VERSION 1
#define CLIENT_ID "-DI-0001-35345343441"
#define BLOCK_LENGTH 16384
#define HANDSHAKE_LENGHT 68
#define SHA1_LENGTH 20

#define PEER_ID_LENGTH 20
#define PEER_SLEEP_TIME 60
#define PEER_MAX_REQUEST_NUMBER 64
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////STRING CONSTS////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
#define TRACKER_STATUS_OK "OK"
#define TRACKER_STATUS_UPDATING "Updating..."
#define TRACKER_STATUS_ERROR "Error"
#define TRACKER_STATUS_TIMEOUT "Timeout"
#define TRACKER_STATUS_UNRESOLVED "Can not resolve domain name"
#define TRACKER_STATUS_CONNECTING "Connecting..."

#define TORRENT_ERROR_IO_ERROR "IO Error"
#define TORRENT_ERROR_INVALID_METAFILE "Invalid torrent file"
#define TORRENT_ERROR_NO_ERROR "No error"
#define TORRENT_ERROR_EXISTS "Torrent is already exists"
#define TORRENT_ERROR_NOT_EXISTS "Torrent is not exists"
#define TORRENT_ERROR_CAN_NOT_START "Can not start torrent"
#define TORRENT_ERROR_NO_STATE_FILE "Can not find state file"

#define GENERAL_ERROR_NO_MEMORY_AVAILABLE "No memory available"
#define GENERAL_ERROR_UNDEF_ERROR "Something wrong"


#endif /* CONSTS_H_ */
