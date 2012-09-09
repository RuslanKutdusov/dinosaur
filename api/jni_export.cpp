/*
 * jni_export.cpp
 *
 *  Created on: 17.08.2012
 *      Author: ruslan
 */

#include "jni_export.h"
#include "conversions.h"

#define UINT16_MAX 0xFFFF
#define UINT32_MAX 0xFFFFFFFF

dinosaur::DinosaurPtr dino;
dinosaur::torrent::MetafilePtr meta;

jobjectArray Java_dinosaur_Dinosaur_InitLibrary
  (JNIEnv * env, jobject jobj)
{
	dinosaur::torrent_failures tfs;
	try
	{
		dinosaur::Dinosaur::CreateDinosaur(dino, tfs);
	}
	catch(dinosaur::SyscallException & e)
	{
		ThrowSyscallException(env, e);
		return NULL;
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
		return NULL;
	}
	return Create_torrent_failures(env, tfs);
}


void Java_dinosaur_Dinosaur_ReleaseLibrary
	(JNIEnv * env, jobject jobj)
{
	if (dino != NULL)
		dinosaur::Dinosaur::DeleteDinosaur(dino);
}

jobject Java_dinosaur_Dinosaur_native_1OpenMetafile
  (JNIEnv * env, jobject jobj, jstring jmetafile_path)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	try
	{
		const char * cmetafile_path = env->GetStringUTFChars(jmetafile_path, NULL);
		std::string metafile_path = cmetafile_path;
		env->ReleaseStringUTFChars( jmetafile_path, cmetafile_path);

		meta.reset(new dinosaur::torrent::Metafile(metafile_path));
		return Create_Metafile(env, meta);
	}
	catch (dinosaur::Exception & e) {
		meta.reset();
		ThrowException(env, e);
		return NULL;
	}
	catch(dinosaur::SyscallException & e)
	{
		meta.reset();
		ThrowSyscallException(env, e);
		return NULL;
	}
	return NULL;
}

void Java_dinosaur_Dinosaur_native_1CloseMetafile
(JNIEnv * env, jobject jobj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	meta.reset();
}

jstring Java_dinosaur_Dinosaur_native_1AddTorrent
  (JNIEnv * env, jobject jobj, jstring jsave_directory)
{
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * csave_directory = env->GetStringUTFChars(jsave_directory, NULL);
	std::string save_directory = csave_directory;
	env->ReleaseStringUTFChars( jsave_directory, csave_directory);

	std::string hash;
	try
	{
		dino->AddTorrent(*meta, save_directory, hash);
	}
	catch(dinosaur::Exception & e)
	{
		meta.reset();
		ThrowException(env, e);
		return NULL;
	}
	meta.reset();
	return env->NewStringUTF(hash.c_str());
}

void Java_dinosaur_Dinosaur_native_1PauseTorrent
	(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dino->PauseTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_native_1ContinueTorrent
	(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dino->ContinueTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_native_1CheckTorrent
	(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL )
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dino->CheckTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_native_1DeleteTorrent
	(JNIEnv * env, jobject jobj, jstring jhash, jboolean with_data)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dino->DeleteTorrent(hash, with_data);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

jobject Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1stat
  (JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::torrent_stat ts;
		dino->get_torrent_info_stat(hash, ts);
		return Create_torrent_stat(env, ts);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobject Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1dyn
(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::torrent_dyn tn;
		dino->get_torrent_info_dyn(hash, tn);
		return Create_torrent_dyn(env, tn);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1trackers
(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::trackers ref;
		dino->get_torrent_info_trackers(hash, ref);
		return Create_trackers(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobject Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1file_1stat
(JNIEnv * env, jobject jobj, jstring jhash, jlong jindex)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::file_stat ref;
		dino->get_torrent_info_file_stat(hash, (dinosaur::FILE_INDEX)jindex, ref);
		return Create_file_stat(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobject Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1file_1dyn
(JNIEnv * env, jobject jobj, jstring jhash, jlong jindex)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
	try
	{
		dinosaur::info::file_dyn ref;
		dino->get_torrent_info_file_dyn(hash, (dinosaur::FILE_INDEX)jindex, ref);
		return Create_file_dyn(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1seeders
(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::peers ref;
		dino->get_torrent_info_seeders(hash, ref);
		return Create_peers(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1leechers
(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::peers ref;
		dino->get_torrent_info_leechers(hash, ref);
		return Create_peers(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1downloadable_1pieces
(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::info::downloadable_pieces ref;
		dino->get_torrent_info_downloadable_pieces(hash, ref);
		return Create_downloadable_pieces(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobject Java_dinosaur_Dinosaur_native_1get_1torrent_1failure_1desc
(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dinosaur::torrent_failure tf;
		dino->get_torrent_failure_desc(hash, tf);
		return Create_torrent_failure(env, tf);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

void Java_dinosaur_Dinosaur_native_1set_1file_1priority
  (JNIEnv * env, jobject jobj, jstring jhash, jlong jindex, jint jprio)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * chash = env->GetStringUTFChars(jhash, NULL);
	std::string hash = chash;
	env->ReleaseStringUTFChars( jhash, chash);
	try
	{
		dino->set_file_priority(hash, (dinosaur::FILE_INDEX)jindex, (dinosaur::DOWNLOAD_PRIORITY)jprio);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1TorrentList
(JNIEnv * env, jobject jobj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	try
	{
		std::list<std::string> ref;
		dino->get_TorrentList(ref);
		return Create_strings(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1active_1torrents(JNIEnv * env, jobject jobj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	try
	{
		std::list<std::string> ref;
		dino->get_active_torrents(ref);
		return Create_strings(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}

jobjectArray Java_dinosaur_Dinosaur_native_1get_1torrents_1in_1queue(JNIEnv * env, jobject jobj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	try
	{
		std::list<std::string> ref;
		dino->get_torrents_in_queue(ref);
		return Create_strings(env, ref);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
	return NULL;
}


jobject Java_dinosaur_Dinosaur_native_1get_1socket_1status
(JNIEnv * env, jobject jobj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	return Create_socket_status(env, dino->get_socket_status());
}

void Java_dinosaur_Dinosaur_native_1UpdateConfigs
  (JNIEnv * env, jobject obj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	try
	{
		dino->UpdateConfigs();
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

jobject Java_dinosaur_Dinosaur_native_1get_1configs
(JNIEnv * env, jobject obj)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return NULL;
	}
	return Create_configs(env, dino);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1download_1directory
  (JNIEnv * env, jobject jobj, jstring jdir)
{
	if (dino == NULL || jdir == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * cdir = env->GetStringUTFChars(jdir, NULL);
	std::string dir = cdir;
	env->ReleaseStringUTFChars( jdir, cdir);
	try
	{
		dino->Config.set_download_directory(dir);
	}
	catch (dinosaur::Exception & e) {
		ThrowException(env, e);
	}
	catch (dinosaur::SyscallException & e) {
		ThrowSyscallException(env, e);
	}
}

void Java_dinosaur_Dinosaur_native_1set_1config_1port
  (JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v < 0 || v > UINT16_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_PORT);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_port(v);
}


JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1write_1cache_1size
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v <= 0 || v > UINT16_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_W_CACHE_SIZE);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_write_cache_size(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1read_1cache_1size
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v <= 0 || v > UINT16_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_R_CACHE_SIZE);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_read_cache_size(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1tracker_1numwant
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v < 0 || v > UINT32_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_TRACKER_NUM_WANT);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_tracker_numwant(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1tracker_1default_1interval
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v <= 0)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_TRACKER_DEF_INTERVAL);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_tracker_default_interval(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1max_1active_1seeders
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v <= 0 || v > UINT32_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_MAX_ACTIVE_SEEDS);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_max_active_seeders(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1max_1active_1leechers
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v <= 0 || v > UINT32_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_MAX_ACTIVE_LEECHS);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_max_active_leechers(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1send_1have
(JNIEnv * env, jobject obj, jboolean v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_send_have(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1listen_1on
(JNIEnv * env, jobject obj, jstring v)
{
	if (dino == NULL || v == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	const char * cv = env->GetStringUTFChars(v, NULL);
	try
	{
		dino->Config.set_listen_on(cv);
	}
	catch (dinosaur::Exception  & e) {
		env->ReleaseStringUTFChars( v, cv);
		ThrowException(env, e);
		return;
	}
	env->ReleaseStringUTFChars( v, cv);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1max_1active_1torrents
(JNIEnv * env, jobject obj, jlong v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v <= 0 || v > UINT16_MAX)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_max_active_torrents(v);
}

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1fin_1ratio
(JNIEnv * env, jobject obj, jfloat v)
{
	if (dino == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	if (v < 0)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_INVALID_FIN_RATIO);
		ThrowException(env, e);
		return;
	}
	dino->Config.set_fin_ratio(v);
}

