/*
 * jni_export.cpp
 *
 *  Created on: 17.08.2012
 *      Author: ruslan
 */

#include "jni_export.h"
#include "conversions.h"

#define UINT16_MAX 65535
#define UINT32_MAX 4294967295

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
	return Create_torrent_failures(env, tfs);
}


void Java_dinosaur_Dinosaur_ReleaseLibrary
	(JNIEnv * env, jobject jobj)
{
	if (dino != NULL)
		dinosaur::Dinosaur::DeleteDinosaur(dino);
}

jobject Java_dinosaur_Dinosaur_OpenMetafile
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
	return NULL;
}

jstring Java_dinosaur_Dinosaur_AddTorrent
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
	catch(dinosaur::SyscallException & e)
	{
		meta.reset();
		ThrowSyscallException(env, e);
		return NULL;
	}
	meta.reset();
	return env->NewStringUTF(hash.c_str());
}

void Java_dinosaur_Dinosaur_StartTorrent
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
		//dino->StartTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_StopTorrent
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
		//dino->StopTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_PauseTorrent
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

void Java_dinosaur_Dinosaur_ContinueTorrent
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

void Java_dinosaur_Dinosaur_CheckTorrent
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

void Java_dinosaur_Dinosaur_DeleteTorrent
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
		dino->DeleteTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

jobject Java_dinosaur_Dinosaur_get_1torrent_1info_1stat
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

jobject Java_dinosaur_Dinosaur_get_1torrent_1info_1dyn
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

jobjectArray Java_dinosaur_Dinosaur_get_1torrent_1info_1trackers
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

jobject Java_dinosaur_Dinosaur_get_1torrent_1info_1file_1stat
(JNIEnv * env, jobject jobj, jstring jhash, jint jindex)
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

jobject Java_dinosaur_Dinosaur_get_1torrent_1info_1file_1dyn
(JNIEnv * env, jobject jobj, jstring jhash, jint jindex)
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

jobjectArray Java_dinosaur_Dinosaur_get_1torrent_1info_1seeders
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

jobjectArray Java_dinosaur_Dinosaur_get_1torrent_1info_1leechers
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

jobjectArray Java_dinosaur_Dinosaur_get_1torrent_1info_1downloadable_1pieces
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

jobjectArray Java_dinosaur_Dinosaur_get_1TorrentList
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

jobject Java_dinosaur_Dinosaur_get_1socket_1status
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

void Java_dinosaur_Dinosaur_UpdateConfigs
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

jobject Java_dinosaur_Dinosaur_get_1configs
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1download_1directory
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

void Java_dinosaur_Dinosaur_set_1config_1port
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


JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1write_1cache_1size
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1read_1cache_1size
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1tracker_1numwant
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1tracker_1default_1interval
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1max_1active_1seeders
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1max_1active_1leechers
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1send_1have
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1listen_1on
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

JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_set_1config_1max_1active_1torrents
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

jobject Java_dinosaur_Dinosaur_test
(JNIEnv * env, jobject jobj)
{
	dinosaur::torrent_failure tf;
	tf.description = "how do you do?";
	tf.errno_ = 234;
	tf.exception_errcode =  dinosaur::Exception::ERR_CODE_CACHE_FULL;
	tf.where = dinosaur::TORRENT_FAILURE_WRITE_FILE;
	dinosaur::Exception e(dinosaur::Exception::ERR_CODE_BLOCK_TOO_BIG);
	ThrowException(env,e);
	return Create_torrent_failure(env, tf);

}

jobjectArray Java_dinosaur_Dinosaur_test2
(JNIEnv * env, jobject jobj)
{
	dinosaur::torrent_failures tfs;
	for(int i = 0; i < 13; i++)
	{
		dinosaur::torrent_failure tf;
		char a[256];
		sprintf(a, "ho%d", i);
		tf.description = a;
		tf.errno_ = i*i ;
		tf.exception_errcode =  (dinosaur::Exception::ERR_CODES)i;
		tf.where = (dinosaur::TORRENT_FAILURE)( i % 6);
		tfs.push_back(tf);
	}
	return Create_torrent_failures(env, tfs);
}

/*
 * Class:     dinosaur_Dinosaur
 * Method:    test_short
 * Signature: ()S
 */
jshort Java_dinosaur_Dinosaur_test_1short
  (JNIEnv *, jobject)
{
	uint16_t a = 45356;
	return a;
}

/*
 * Class:     dinosaur_Dinosaur
 * Method:    test_int
 * Signature: ()I
 */
jint Java_dinosaur_Dinosaur_test_1int
  (JNIEnv *, jobject)
{
	uint32_t a = 3000000000;
	return a;
}

/*
 * Class:     dinosaur_Dinosaur
 * Method:    test_long
 * Signature: ()J
 */
jlong Java_dinosaur_Dinosaur_test_1long
  (JNIEnv *, jobject)
{
	uint64_t a = 5000000000;
	return a;
}

