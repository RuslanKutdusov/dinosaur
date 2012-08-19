/*
 * conversions.cpp
 *
 *  Created on: 18.08.2012
 *      Author: ruslan
 */

#include "conversions.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Exception//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ThrowException(JNIEnv * env, dinosaur::Exception & e)
{
	jclass clazz = env->FindClass("dinosaur/DinosaurException");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(I)V");
	env->Throw((jthrowable)env->NewObject(clazz, ID, e.get_errcode()));
}

void ThrowSyscallException(JNIEnv * env, dinosaur::SyscallException & e)
{
	jclass clazz = env->FindClass("dinosaur/DinosaurException");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(I)V");
	env->Throw((jthrowable)env->NewObject(clazz, ID, e.get_errno()));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//object/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_object(JNIEnv * env, jclass clazz)
{
	jmethodID def_constructor = env->GetMethodID(clazz, "<init>", "()V");
	return env->NewObject(clazz, def_constructor);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//torrent_failure////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


jobject Create_torrent_failure(JNIEnv * env, const dinosaur::torrent_failure & tf)
{
	jclass clazz = env->FindClass("dinosaur/torrent_failure");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(IIILjava/lang/String;)V");
	return env->NewObject(clazz, ID, tf.exception_errcode, tf.errno_, tf.where, env->NewStringUTF(tf.description.c_str()));
}

jobject Create_torrent_failure(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::torrent_failure & tf)
{
	return env->NewObject(clazz, ID, tf.exception_errcode, tf.errno_, tf.where, env->NewStringUTF(tf.description.c_str()));
}

jobjectArray Create_torrent_failures(JNIEnv * env, const dinosaur::torrent_failures & tfs)
{
	size_t size = tfs.size();
	jclass clazz = env->FindClass("dinosaur/torrent_failure");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(IIILjava/lang/String;)V");
	jobjectArray arr = env->NewObjectArray(size, clazz, Create_object(env, clazz));
	size_t i = 0;
	for(dinosaur::torrent_failures::const_iterator iter = tfs.begin(); iter != tfs.end(); ++iter)
		env->SetObjectArrayElement(arr, i++, Create_torrent_failure(env, clazz, constructor, *iter));
	return arr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//file_stat///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_file_stat(JNIEnv * env, dinosaur::info::file_stat & file_stat)
{
	jclass clazz = env->FindClass("dinosaur/info/file_stat");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJ)V");
	return env->NewObject(clazz, ID, env->NewStringUTF(file_stat.path.c_str()), file_stat.length, file_stat.index);
}

jobject Create_file_stat(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::file_stat & file_stat)
{
	return env->NewObject(clazz, ID, env->NewStringUTF(file_stat.path.c_str()), file_stat.length, file_stat.index);
}

jobjectArray Create_file_stats(JNIEnv * env, const dinosaur::info::files_stat & files_stat)
{
	size_t size = files_stat.size();
	jclass clazz = env->FindClass("dinosaur/info/file_stat");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJ)V");
	jobjectArray arr = env->NewObjectArray(size, clazz, Create_object(env, clazz));
	for(size_t i = 0; i < size; i++)
		env->SetObjectArrayElement(arr, i, Create_file_stat(env, clazz, constructor, files_stat[i]));
	return arr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//std::string////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobjectArray Create_strings(JNIEnv * env, const std::vector<std::string> & strings)
{
	jobjectArray arr = (jobjectArray)env->NewObjectArray(strings.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));

	for(size_t i = 0; i < strings.size(); i++)
		env->SetObjectArrayElement(arr, i, env->NewStringUTF(strings[i].c_str()));
	return arr;
}

jobjectArray Create_strings(JNIEnv * env, const std::list<std::string> & strings)
{
	jobjectArray arr = (jobjectArray)env->NewObjectArray(strings.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));

	size_t i = 0;
	for(std::list<std::string>::const_iterator iter = strings.begin(); iter != strings.end(); ++iter)
		env->SetObjectArrayElement(arr, i++, env->NewStringUTF(iter->c_str()));
	return arr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Metafile///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_Metafile(JNIEnv * env, dinosaur::torrent::MetafilePtr & metafile)
{
	jclass metafile_clazz = env->FindClass("dinosaur/Metafile");
	jmethodID ID = env->GetMethodID(metafile_clazz, "<init>", "([Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJJ[Ldinosaur/info/file_stat;Ljava/lang/String;JILjava/lang/String;)V");
	return env->NewObject(metafile_clazz, ID, Create_strings(env, metafile->announces),
											env->NewStringUTF(metafile->comment.c_str()),
											env->NewStringUTF(metafile->created_by.c_str()),
											metafile->creation_date,
											metafile->private_,
											metafile->length,
											Create_file_stats(env, metafile->files),
											env->NewStringUTF(metafile->name.c_str()),
											metafile->piece_length,
											metafile->piece_count,
											env->NewStringUTF(metafile->info_hash_hex));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::torrent_stat/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_torrent_stat(JNIEnv * env, dinosaur::info::torrent_stat & ts)
{
	jclass clazz = env->FindClass("dinosaur/info/torrent_stat");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJJJILjava/lang/String;IJ)V");
	return env->NewObject(clazz, ID,
									env->NewStringUTF(ts.name.c_str()),
									env->NewStringUTF(ts.comment.c_str()),
									env->NewStringUTF(ts.created_by.c_str()),
									env->NewStringUTF(ts.download_directory.c_str()),
									ts.creation_date,
									ts.private_,
									ts.length,
									ts.piece_length,
									ts.piece_count,
									env->NewStringUTF(ts.info_hash_hex),
									ts.start_time,
									ts.files_count);


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::torrent_dyn//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_torrent_dyn(JNIEnv * env, dinosaur::info::torrent_dyn & tn)
{
	jclass clazz = env->FindClass("dinosaur/info/torrent_dyn");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(JJIIIIIIIIF)V");
	return env->NewObject(clazz, ID, tn.downloaded,
										tn.uploaded,
										tn.rx_speed,
										tn.tx_speed,
										tn.seeders,
										tn.total_seeders,
										tn.leechers,
										tn.progress,
										tn.work,
										tn.remain_time,
										tn.ratio);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::downloadable_piece//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_downloadable_piece(JNIEnv * env, const dinosaur::info::downloadable_piece & dp)
{
	jclass clazz = env->FindClass("dinosaur/info/downloadable_piece");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(IIII)V");
	return Create_downloadable_piece(env, clazz, ID, dp);
}

jobject Create_downloadable_piece(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::downloadable_piece & dp)
{
	return env->NewObject(clazz, ID, dp.index,
											dp.block2download,
											dp.downloaded_blocks,
											dp.priority);
}

jobjectArray Create_downloadable_pieces(JNIEnv * env, const dinosaur::info::downloadable_pieces & dps)
{
	size_t size = dps.size();
	jclass clazz = env->FindClass("dinosaur/info/downloadable_piece");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(IIII)V");
	jobjectArray arr = env->NewObjectArray(size, clazz, Create_object(env, clazz));
	size_t i = 0;
	for(dinosaur::info::downloadable_pieces::const_iterator iter = dps.begin(); iter != dps.end(); ++iter)
		env->SetObjectArrayElement(arr, i++, Create_downloadable_piece(env, clazz, constructor, *iter));
	return arr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::file_dyn/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_file_dyn(JNIEnv * env, dinosaur::info::file_dyn & fd)
{
	jclass clazz = env->FindClass("dinosaur/info/file_dyn");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(IJ)V");
	return env->NewObject(clazz, ID, fd.priority,
										fd.downloaded);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::peer//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_peer(JNIEnv * env, const dinosaur::info::peer & p)
{
	jclass clazz = env->FindClass("dinosaur/info/peer");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJIIDIIZ)V");
	return Create_peer(env, clazz, ID, p);
}

jobject Create_peer(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::peer & p)
{
	return env->NewObject(clazz, ID, env->NewStringUTF(p.ip),
										p.downloaded,
										p.uploaded,
										p.downSpeed,
										p.upSpeed,
										p.available,
										p.blocks2request,
										p.requested_blocks,
										p.may_request);
}

jobjectArray Create_peers(JNIEnv * env, const dinosaur::info::peers & ps)
{
	size_t size = ps.size();
	jclass clazz = env->FindClass("dinosaur/info/peer");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJIIDIIZ)V");
	jobjectArray arr = env->NewObjectArray(size, clazz, Create_object(env, clazz));
	size_t i = 0;
	for(dinosaur::info::peers::const_iterator iter = ps.begin(); iter != ps.end(); ++iter)
		env->SetObjectArrayElement(arr, i++, Create_peer(env, clazz, constructor, *iter));
	return arr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::tracker//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_tracker(JNIEnv * env, const dinosaur::info::tracker & t)
{
	jclass clazz = env->FindClass("dinosaur/info/tracker");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;ILjava/lang/String;JJI)V");
	return Create_tracker(env, clazz, ID, t);
}

jobject Create_tracker(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::tracker & t)
{
	return env->NewObject(clazz, ID, env->NewStringUTF(t.announce.c_str()),
										t.status,
										env->NewStringUTF(t.failure_mes.c_str()),
										t.seeders,
										t.leechers,
										t.update_in);
}

jobjectArray Create_trackers(JNIEnv * env, const dinosaur::info::trackers & ts)
{
	size_t size = ts.size();
	jclass clazz = env->FindClass("dinosaur/info/tracker");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;ILjava/lang/String;JJI)V");
	jobjectArray arr = env->NewObjectArray(size, clazz, Create_object(env, clazz));
	size_t i = 0;
	for(dinosaur::info::trackers::const_iterator iter = ts.begin(); iter != ts.end(); ++iter)
		env->SetObjectArrayElement(arr, i++, Create_tracker(env, clazz, constructor, *iter));
	return arr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//socket_status/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_socket_status(JNIEnv * env, const dinosaur::socket_status & ss)
{
	jclass clazz = env->FindClass("dinosaur/socket_status");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(ZII)V");
	return env->NewObject(clazz, ID, ss.listen,
										ss.errno_,
										ss.exception_errcode);
}

