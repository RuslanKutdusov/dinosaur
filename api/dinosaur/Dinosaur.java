package dinosaur;

import dinosaur.info.*;

public class Dinosaur {
	public Dinosaur()
	{
		
	}
	public native torrent_failure[] InitLibrary() throws DinosaurSyscallException;
	public native void ReleaseLibrary();
	public native Metafile OpenMetafile(String metafile_path) throws DinosaurException;
	public native String AddTorrent(String save_directory) throws DinosaurException;
	public native void StartTorrent(String hash) throws DinosaurException;
	public native void StopTorrent(String hash) throws DinosaurException;
	public native void PauseTorrent(String hash) throws DinosaurException;
	public native void ContinueTorrent(String hash) throws DinosaurException;
	public native void CheckTorrent(String hash) throws DinosaurException;
	public native void DeleteTorrent(String hash) throws DinosaurException;
	public native torrent_stat get_torrent_info_stat(String hash) throws DinosaurException;
	public native torrent_dyn get_torrent_info_dyn(String hash) throws DinosaurException;
	public native tracker[] get_torrent_info_trackers(String hash) throws DinosaurException;
	public native file_stat get_torrent_info_file_stat(String hash, long index) throws DinosaurException;
	public native file_dyn get_torrent_info_file_dyn(String hash, long index) throws DinosaurException;
	public native peer[] get_torrent_info_seeders(String hash) throws DinosaurException;
	public native peer[] get_torrent_info_leechers(String hash) throws DinosaurException;
	public native downloadable_piece[] get_torrent_info_downloadable_pieces(String hash) throws DinosaurException;
	public native void set_file_priority(String hash, long index, int prio)  throws DinosaurException;
	public void set_file_priority(String hash, long index, DOWNLOAD_PRIORITY prio)  throws DinosaurException
	{
		set_file_priority(hash, index, prio.ordinal());
	}
	public native String[] get_TorrentList() throws DinosaurException;
	public native socket_status get_socket_status() throws DinosaurException;
	public native void UpdateConfigs()  throws DinosaurException;
	public native Configs get_configs() throws DinosaurException;
	public native void set_config_download_directory(String dir) throws DinosaurException, DinosaurSyscallException;
	public native void set_config_port(long v) throws DinosaurException;
	public native void set_config_write_cache_size(long v) throws DinosaurException;
	public native void set_config_read_cache_size(long v) throws DinosaurException;
	public native void set_config_tracker_numwant(long v) throws DinosaurException;
	public native void set_config_tracker_default_interval(long v) throws DinosaurException;
	public native void set_config_max_active_seeders(long v) throws DinosaurException;
	public native void set_config_max_active_leechers(long v) throws DinosaurException;
	public native void set_config_send_have(boolean v) throws DinosaurException;
	public native void set_config_listen_on(String ip) throws DinosaurException;
	public native void set_config_max_active_torrents(long v) throws DinosaurException;
	
	public native torrent_failure test() throws DinosaurException;
	public native torrent_failure[] test2();
	
	public native short test_short();
	public native int test_int();
	public native long test_long();
	static
	{
		System.loadLibrary("dinosaur");
	}
}
