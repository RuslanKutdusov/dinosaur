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
