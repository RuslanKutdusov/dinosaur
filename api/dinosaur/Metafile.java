package dinosaur;

import dinosaur.info.file_stat;

public class Metafile {
	public String[] 	announces;
	public String 		comment;
	public String 		created_by;
	public long 		creation_date;
	public long 		private_;
	public long 		length;
	public file_stat[] 	files;
	public String 		name;
	public long 		piece_length;
	public long			piece_count;
	public String 		infohash;
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
