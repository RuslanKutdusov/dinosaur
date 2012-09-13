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
	jclass clazz = env->FindClass("dinosaur/DinosaurSyscallException");
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
	return env->NewObject(clazz, ID, env->NewStringUTF(file_stat.path.c_str()), (jlong)file_stat.length, (jlong)file_stat.index);
}

jobject Create_file_stat(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::file_stat & file_stat)
{
	return env->NewObject(clazz, ID, env->NewStringUTF(file_stat.path.c_str()), (jlong)file_stat.length, (jlong)file_stat.index);
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
	jmethodID ID = env->GetMethodID(metafile_clazz, "<init>", "([Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJJ[Ldinosaur/info/file_stat;Ljava/lang/String;JJLjava/lang/String;)V");
	return env->NewObject(metafile_clazz, ID, Create_strings(env, metafile->announces),
											env->NewStringUTF(metafile->comment.c_str()),
											env->NewStringUTF(metafile->created_by.c_str()),
											(jlong)metafile->creation_date,
											(jlong)metafile->private_,
											(jlong)metafile->length,
											Create_file_stats(env, metafile->files),
											env->NewStringUTF(metafile->name.c_str()),
											(jlong)metafile->piece_length,
											(jlong)metafile->piece_count,
											env->NewStringUTF(metafile->info_hash_hex.c_str()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::torrent_stat/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_torrent_stat(JNIEnv * env, dinosaur::info::torrent_stat & ts)
{
	jclass clazz = env->FindClass("dinosaur/info/torrent_stat");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJJJJLjava/lang/String;JJ)V");
	return env->NewObject(clazz, ID,
									env->NewStringUTF(ts.name.c_str()),
									env->NewStringUTF(ts.comment.c_str()),
									env->NewStringUTF(ts.created_by.c_str()),
									env->NewStringUTF(ts.download_directory.c_str()),
									(jlong)ts.creation_date,
									(jlong)ts.private_,
									(jlong)ts.length,
									(jlong)ts.piece_length,
									(jlong)ts.piece_count,
									env->NewStringUTF(ts.info_hash_hex.c_str()),
									(jlong)ts.start_time,
									(jlong)ts.files_count);


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::torrent_dyn//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_torrent_dyn(JNIEnv * env, dinosaur::info::torrent_dyn & tn)
{
	jclass clazz = env->FindClass("dinosaur/info/torrent_dyn");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(JJJJJJJJIJF)V");
	return env->NewObject(clazz, ID, (jlong)tn.downloaded,
										(jlong)tn.uploaded,
										(jlong)tn.rx_speed,
										(jlong)tn.tx_speed,
										(jlong)tn.seeders,
										(jlong)tn.total_seeders,
										(jlong)tn.leechers,
										(jlong)tn.progress,
										(jlong)tn.work,
										(jlong)tn.remain_time,
										tn.ratio);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::downloadable_piece//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_downloadable_piece(JNIEnv * env, const dinosaur::info::downloadable_piece & dp)
{
	jclass clazz = env->FindClass("dinosaur/info/downloadable_piece");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(JJJI)V");
	return Create_downloadable_piece(env, clazz, ID, dp);
}

jobject Create_downloadable_piece(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::downloadable_piece & dp)
{
	return env->NewObject(clazz, ID, (jlong)dp.index,
											(jlong)dp.block2download,
											(jlong)dp.downloaded_blocks,
											dp.priority);
}

jobjectArray Create_downloadable_pieces(JNIEnv * env, const dinosaur::info::downloadable_pieces & dps)
{
	size_t size = dps.size();
	jclass clazz = env->FindClass("dinosaur/info/downloadable_piece");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(JJJI)V");
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
									(jlong)fd.downloaded);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//info::peer//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jobject Create_peer(JNIEnv * env, const dinosaur::info::peer & p)
{
	jclass clazz = env->FindClass("dinosaur/info/peer");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJJJDJJZ)V");
	return Create_peer(env, clazz, ID, p);
}

jobject Create_peer(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::peer & p)
{
	return env->NewObject(clazz, ID, env->NewStringUTF(p.ip),
										(jlong)p.downloaded,
										(jlong)p.uploaded,
										(jlong)p.downSpeed,
										(jlong)p.upSpeed,
										p.available,
										(jlong)p.blocks2request,
										(jlong)p.requested_blocks,
										p.may_request);
}

jobjectArray Create_peers(JNIEnv * env, const dinosaur::info::peers & ps)
{
	size_t size = ps.size();
	jclass clazz = env->FindClass("dinosaur/info/peer");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJJJDJJZ)V");
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
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;ILjava/lang/String;JJJ)V");
	return Create_tracker(env, clazz, ID, t);
}

jobject Create_tracker(JNIEnv * env, jclass clazz, jmethodID ID, const dinosaur::info::tracker & t)
{
	return env->NewObject(clazz, ID, env->NewStringUTF(t.announce.c_str()),
										t.status,
										env->NewStringUTF(t.failure_mes.c_str()),
										(jlong)t.seeders,
										(jlong)t.leechers,
										(jlong)t.update_in);
}

jobjectArray Create_trackers(JNIEnv * env, const dinosaur::info::trackers & ts)
{
	size_t size = ts.size();
	jclass clazz = env->FindClass("dinosaur/info/tracker");
	jmethodID constructor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;ILjava/lang/String;JJJ)V");
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

jobject Create_configs(JNIEnv * env, const dinosaur::DinosaurPtr & ptr)
{
	jclass clazz = env->FindClass("dinosaur/Configs");
	jmethodID ID = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;JJJJJJJZLjava/lang/String;JF)V");
	dinosaur::IP_CHAR ip;
	ptr->Config.get_listen_on(ip);
	return env->NewObject(clazz, ID, env->NewStringUTF(ptr->Config.get_download_directory().c_str()),
									(jlong)ptr->Config.get_port(),
									(jlong)ptr->Config.get_write_cache_size(),
									(jlong)ptr->Config.get_read_cache_size(),
									(jlong)ptr->Config.get_tracker_default_interval(),
									(jlong)ptr->Config.get_tracker_numwant(),
									(jlong)ptr->Config.get_max_active_seeders(),
									(jlong)ptr->Config.get_max_active_leechers(),
									ptr->Config.get_send_have(),
									env->NewStringUTF(ip),
									(jlong)ptr->Config.get_max_active_torrents(),
									(jfloat)ptr->Config.get_fin_ratio());
}


