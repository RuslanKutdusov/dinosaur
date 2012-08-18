/*
 * jni_export.cpp
 *
 *  Created on: 17.08.2012
 *      Author: ruslan
 */

#include "jni_export.h"
#include "conversions.h"

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
		std::string metafile_path = env->GetStringUTFChars(jmetafile_path, NULL);
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
	std::string save_directory = env->GetStringUTFChars(jsave_directory, NULL);
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
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
	try
	{
		dino->StartTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_StopTorrent
	(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
	try
	{
		dino->StopTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
}

void Java_dinosaur_Dinosaur_PauseTorrent
	(JNIEnv * env, jobject jobj, jstring jhash)
{
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
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
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
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
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
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
	if (dino == NULL || meta == NULL)
	{
		dinosaur::Exception e(dinosaur::Exception::ERR_CODE_NULL_REF);
		ThrowException(env, e);
		return;
	}
	std::string hash = env->GetStringUTFChars(jhash, NULL);
	try
	{
		dino->DeleteTorrent(hash);
	}
	catch(dinosaur::Exception & e)
	{
		ThrowException(env, e);
	}
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
