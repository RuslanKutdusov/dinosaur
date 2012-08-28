package dinosaur;

import dinosaur.info.file_stat;

public class Metafile {
	public String[] 	announces; //список аннонсов(трекеров)
	public String 		comment;//комментарий к раздаче
	public String 		created_by;//создан в ...
	public long 		creation_date;//дата создания
	public long 		private_;//приватный ии нет
	public long 		length;//размер
	public file_stat[] 	files;//список файлов
	public String 		name;//назвние раздачи
	public long 		piece_length;//рзмер куска
	public long			piece_count;//кол-во кусков
	public String 		infohash;//хэш
	public Metafile() {
		// TODO Auto-generated constructor stub
	}
	public Metafile(String[] announces, String comment, String created_by, long creation_date,
					long private_, long length, file_stat[] files, String name, long piece_length,
					long piece_count, String infohash)
	{
		this.announces = announces;
		this.comment = comment;
		this.created_by = created_by;
		this.creation_date = creation_date;
		this.private_ = private_;
		this.length = length;
		this.files = files;
		this.name = name;
		this.piece_length = piece_length;
		this.piece_count = piece_count;
		this.infohash = infohash;
	}
}
