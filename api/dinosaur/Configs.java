package dinosaur;

public class Configs {
	String 			download_directory;
	long 			port;
	long 			write_cache_size;
	long 			read_cache_size;
	long 			tracker_default_interval;
	long			tracker_numwant;
	long 			max_active_seeders;
	long 			max_active_leechers;
	boolean			send_have;
	String 			listen_on;
	long 			max_active_torrents;
	public Configs() {
		// TODO Auto-generated constructor stub
	}
	public Configs(String download_directory, long port, long write_cache_size, long read_cache_size,
					long tracker_def_interval, long tracker_numwant, long max_active_seeders, 
					long max_active_leechers, boolean send_have, String listen_on, long max_active_torrents)
	{
		this.download_directory = download_directory;
		this.port = port;
		this.write_cache_size = write_cache_size;
		this.read_cache_size = read_cache_size;
		this.tracker_default_interval = tracker_def_interval;
		this.tracker_numwant = tracker_numwant;
		this.max_active_seeders = max_active_seeders;
		this.max_active_leechers = max_active_leechers;
		this.send_have =	send_have;
		this.listen_on = listen_on;
		this.max_active_torrents = max_active_torrents;
	}
}
