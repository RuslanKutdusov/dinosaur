package dinosaur;

public class torrent_failure {
	public enum TORRENT_FAILURE
	{
		TORRENT_FAILURE_NONE,//ошибки
		TORRENT_FAILURE_INIT_TORRENT,//ошибка при инициализации торрента
		TORRENT_FAILURE_WRITE_FILE,//при записи в файл
		TORRENT_FAILURE_READ_FILE,//при чтении файла
		TORRENT_FAILURE_GET_BLOCK_CACHE,//при работе с кэшем
		TORRENT_FAILURE_PUT_BLOCK_CACHE,//тоже
		TORRENT_FAILURE_DOWNLOADING//ошибка в алгоритме загрузки
	};

	public DinosaurException.ERR_CODES		exception_errcode;//описание ошибки
	public int								errno_;//описание ошибки
	public TORRENT_FAILURE					where;//где произошла
	public String							description;//здесь может быть имя файла, при работе с которым произошла ошибка
	public torrent_failure()
	{
		
	}
	public torrent_failure(int exception_errcode, int errno_,
							int where, String description)
	{
		this.exception_errcode = DinosaurException.ERR_CODES.values()[exception_errcode];
		this.errno_ = errno_;
		this.where = TORRENT_FAILURE.values()[where];
		this.description = description;
	}
}
