package dinosaur.info;

public class file_stat {
	public String 		path;
	public long			length;
	public long			index;
	public file_stat()
	{
		
	}
	public file_stat(String path, long length, long index)
	{
		this.path = path;
		this.length = length;
		this.index = index;
	}
}
