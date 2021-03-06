/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class dinosaur_Dinosaur */

#ifndef _Included_dinosaur_Dinosaur
#define _Included_dinosaur_Dinosaur
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     dinosaur_Dinosaur
 * Method:    InitLibrary
 * Signature: ()[Ldinosaur/torrent_failure;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_InitLibrary
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    ReleaseLibrary
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_ReleaseLibrary
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_OpenMetafile
 * Signature: (Ljava/lang/String;)Ldinosaur/Metafile;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1OpenMetafile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_CloseMetafile
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1CloseMetafile
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_AddTorrent
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_dinosaur_Dinosaur_native_1AddTorrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_PauseTorrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1PauseTorrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_ContinueTorrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1ContinueTorrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_CheckTorrent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1CheckTorrent
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_DeleteTorrent
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1DeleteTorrent
  (JNIEnv *, jobject, jstring, jboolean);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_stat
 * Signature: (Ljava/lang/String;)Ldinosaur/info/torrent_stat;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1stat
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_dyn
 * Signature: (Ljava/lang/String;)Ldinosaur/info/torrent_dyn;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1dyn
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_trackers
 * Signature: (Ljava/lang/String;)[Ldinosaur/info/tracker;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1trackers
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_file_stat
 * Signature: (Ljava/lang/String;J)Ldinosaur/info/file_stat;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1file_1stat
  (JNIEnv *, jobject, jstring, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_file_dyn
 * Signature: (Ljava/lang/String;J)Ldinosaur/info/file_dyn;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1file_1dyn
  (JNIEnv *, jobject, jstring, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_seeders
 * Signature: (Ljava/lang/String;)[Ldinosaur/info/peer;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1seeders
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_leechers
 * Signature: (Ljava/lang/String;)[Ldinosaur/info/peer;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1leechers
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_info_downloadable_pieces
 * Signature: (Ljava/lang/String;)[Ldinosaur/info/downloadable_piece;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1info_1downloadable_1pieces
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrent_failure_desc
 * Signature: (Ljava/lang/String;)Ldinosaur/torrent_failure;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1torrent_1failure_1desc
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_file_priority
 * Signature: (Ljava/lang/String;JI)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1file_1priority
  (JNIEnv *, jobject, jstring, jlong, jint);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_TorrentList
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1TorrentList
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_active_torrents
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1active_1torrents
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_torrents_in_queue
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_dinosaur_Dinosaur_native_1get_1torrents_1in_1queue
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_socket_status
 * Signature: ()Ldinosaur/socket_status;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1socket_1status
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_UpdateConfigs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1UpdateConfigs
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_get_configs
 * Signature: ()Ldinosaur/Configs;
 */
JNIEXPORT jobject JNICALL Java_dinosaur_Dinosaur_native_1get_1configs
  (JNIEnv *, jobject);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_download_directory
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1download_1directory
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_port
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1port
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_write_cache_size
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1write_1cache_1size
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_read_cache_size
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1read_1cache_1size
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_tracker_numwant
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1tracker_1numwant
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_tracker_default_interval
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1tracker_1default_1interval
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_max_active_seeders
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1max_1active_1seeders
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_max_active_leechers
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1max_1active_1leechers
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_send_have
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1send_1have
  (JNIEnv *, jobject, jboolean);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_listen_on
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1listen_1on
  (JNIEnv *, jobject, jstring);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_max_active_torrents
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1max_1active_1torrents
  (JNIEnv *, jobject, jlong);

/*
 * Class:     dinosaur_Dinosaur
 * Method:    native_set_config_fin_ratio
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_dinosaur_Dinosaur_native_1set_1config_1fin_1ratio
  (JNIEnv *, jobject, jfloat);

#ifdef __cplusplus
}
#endif
#endif
