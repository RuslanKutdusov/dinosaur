package dinosaur.info;

public class torrent_stat {
	public String 		name;//название торрента
	public String 		comment;//комментарий
	public String 		created_by;//создан в ...
	public String 		download_directory;//расположение
	public long 		creation_date;//когда создан
	public long 		private_;//приватный или нет
	public long 		length;//размер
	public long 		piece_length;//размер куска
	public long 		piece_count;//кол-во кусков
	public String 		info_hash_hex;//хэш
	public long			start_time;//когда запущен
	public long			files_count;//кол-во файлов
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
