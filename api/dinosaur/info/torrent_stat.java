package dinosaur.info;

public class torrent_stat {
	public String 		name;
	public String 		comment;
	public String 		created_by;
	public String 		download_directory;
	public long 		creation_date;
	public long 		private_;
	public long 		length;
	public long 		piece_length;
	public long 		piece_count;
	public String 		info_hash_hex;
	public long			start_time;
	public long			files_count;
	public torrent_stat()
	{
		
	}
	public torrent_stat(String name, String comment, String created_by, String download_directory,
						long creation_date, long private_, long length, long piece_length, long piece_count,
						String infohash_hex, long start_time, long files_count)
	{
		this.name = name;
		this.comment = comment;
		this.created_by = created_by;
		this.download_directory = download_directory;
		this.creation_date = creation_date;
		this.private_ = private_;
		this.length = length;
		this.piece_length = piece_length;
		this.piece_count = piece_count;
		this.info_hash_hex = infohash_hex;
		this.start_time = start_time;
		this.files_count = files_count;
	}
}
