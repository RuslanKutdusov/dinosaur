package dinosaur.info;

public class torrent_dyn {
	public enum TORRENT_WORK
	{
		TORRENT_WORK_DOWNLOADING,//качает
		TORRENT_WORK_UPLOADING,//отдает
		TORRENT_WORK_CHECKING,//проверяет
		TORRENT_WORK_PAUSED,//приостановлен
		TORRENT_WORK_FAILURE,//произошла ошибка, вызови get_torrent_failure_desc чтобы узнать подробности
		TORRENT_WORK_RELEASING,//высвобождается
		TORRENT_WORK_DONE//завершен 
	};
	public long 		downloaded;//загружено байт
	public long 		uploaded;//отдано байт
	public long 		rx_speed;//скорость загрузки
	public long 		tx_speed;//скорость отдачи
	public long			seeders;//всего сидов
	public long			total_seeders;//у скольких сидов сейчас качаем
	public long			leechers;//личеры
	public long 		progress;//прогресс 0 - 100 %
	public TORRENT_WORK	work;//чем сейчас занимается торрент
	public long			remain_time;//осталось времени, чтобы скачать
	public float		ratio;//коэффициент = "отдано" поделить на "загружено"
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
