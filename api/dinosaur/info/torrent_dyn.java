package dinosaur.info;

public class torrent_dyn {
	public enum TORRENT_WORK
	{
		TORRENT_WORK_DOWNLOADING,
		TORRENT_WORK_UPLOADING,
		TORRENT_WORK_CHECKING,
		TORRENT_WORK_PAUSED,
		TORRENT_WORK_FAILURE
	};
	public long 		downloaded;
	public long 		uploaded;
	public double 		rx_speed;
	public double 		tx_speed;
	public int			seeders;
	public int			total_seeders;
	public int			leechers;
	public int 			progress;
	public TORRENT_WORK	work;
	public int			remain_time;
	public float		ratio;
	public torrent_dyn()
	{
		
	}
	public torrent_dyn(long downloaded, long uploaded, double rx_speed, double tx_speed, int seeders,
						int total_seeders, int leechers, int progress, int work, int remain_time,
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
