package dinosaur;

public class torrent_failure {
	public enum TORRENT_FAILURE
	{
		TORRENT_FAILURE_INIT_TORRENT,
		TORRENT_FAILURE_WRITE_FILE,
		TORRENT_FAILURE_READ_FILE,
		TORRENT_FAILURE_GET_BLOCK_CACHE,
		TORRENT_FAILURE_PUT_BLOCK_CACHE,
		TORRENT_FAILURE_DOWNLOADING
	};

	public DinosaurException.ERR_CODES		exception_errcode;
	public int								errno_;
	public TORRENT_FAILURE					where;
	public String							description;
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
