package dinosaur.info;

public class torrent_dyn {
	public enum TORRENT_WORK
	{
		TORRENT_WORK_DOWNLOADING,
		TORRENT_WORK_UPLOADING,
		TORRENT_WORK_CHECKING,
		TORRENT_WORK_PAUSED,
		TORRENT_WORK_FAILURE,
		TORRENT_WORK_RELEASING,
		TORRENT_WORK_DONE
	};
	public long 		downloaded;
	public long 		uploaded;
	public long 		rx_speed;
	public long 		tx_speed;
	public long			seeders;
	public long			total_seeders;
	public long			leechers;
	public long 		progress;
	public TORRENT_WORK	work;
	public long			remain_time;
	public float		ratio;
	public torrent_dyn()
	{
		
	}
	public torrent_dyn(long downloaded, long uploaded, long rx_speed, long tx_speed, long seeders,
			long total_seeders, long leechers, long progress, int work, long remain_time,
						float ratio)
	{
		this.downloaded = downloaded;
		this.uploaded = uploaded;
		this.rx_speed = rx_speed;
		this.tx_speed = tx_speed;
		this.seeders = seeders;
		this.total_seeders = total_seeders;
		this.leechers = leechers;
		this.progress = progress;
		this.work = TORRENT_WORK.values()[work];
		this.remain_time = remain_time;
		this.ratio = ratio;
	}
}
