/*
 * conversions.h
 *
 *  Created on: 18.08.2012
 *      Author: ruslan
 */

#ifndef CONVERSIONS_H_
#define CONVERSIONS_H_
#include <jni.h>
#include "../libdinosaur/dinosaur.h"
#include "../libdinosaur/torrent/torrent.h"
#include "../libdinosaur/torrent/torrent_types.h"
#include "../libdinosaur/exceptions/exceptions.h"

void ThrowException(JNIEnv * env, dinosaur::Exception & e);
void ThrowSyscallException(JNIEnv * env, dinosaur::SyscallException & e);

jobject Create_object(JNIEnv * env, jclass clazz);

jobject Create_torrent_failure(JNIEnv * env, const dinosaur::torrent_failure & tf);
jobject Create_torrent_failure(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::torrent_failure & tf);
jobjectArray Create_torrent_failures(JNIEnv * env, const dinosaur::torrent_failures & tfs);

jobject Create_file_stat(JNIEnv * env, dinosaur::info::file_stat & file_stat);
jobject Create_file_stat(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::file_stat & file_stat);
jobjectArray Create_file_stats(JNIEnv * env, const dinosaur::info::files_stat & files_stat);

jobjectArray Create_strings(JNIEnv * env, const std::vector<std::string> & strings);

jobject Create_Metafile(JNIEnv * env, dinosaur::torrent::MetafilePtr & metafile);

jobject Create_torrent_stat(JNIEnv * env, dinosaur::info::torrent_stat & ts);
jobject Create_torrent_dyn(JNIEnv * env, dinosaur::info::torrent_dyn & tn);

jobject Create_downloadable_piece(JNIEnv * env, const dinosaur::info::downloadable_piece & dp);
jobject Create_downloadable_piece(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::downloadable_piece & dp);
jobjectArray Create_downloadable_pieces(JNIEnv * env, const dinosaur::info::downloadable_pieces & dps);

jobject Create_file_dyn(JNIEnv * env, dinosaur::info::file_dyn & fd);

jobject Create_peer(JNIEnv * env, const dinosaur::info::peer & p);
jobject Create_peer(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::peer & p);
jobjectArray Create_peers(JNIEnv * env, const dinosaur::info::peers & ps);

jobject Create_tracker(JNIEnv * env, const dinosaur::info::tracker & t);
jobject Create_tracker(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::tracker & t);
jobjectArray Create_trackers(JNIEnv * env, const dinosaur::info::trackers & ts);

jobject Create_socket_status(JNIEnv * env, dinosaur::socket_status & ss);



#endif /* CONVERSIONS_H_ */
