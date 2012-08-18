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



#endif /* CONVERSIONS_H_ */
