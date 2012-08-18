package dinosaur;

public class Dinosaur {
	public Dinosaur()
	{
		
	}
	public native torrent_failure[] InitLibrary() throws DinosaurSyscallException;
	public native void ReleaseLibrary();
	public native Metafile OpenMetafile(String metafile_path) throws DinosaurException;
	public native String AddTorrent(String save_directory) throws DinosaurException;
	public native void StartTorrent(String hash) throws DinosaurException;
	public native void StopTorrent(String hash) throws DinosaurException;
	public native void PauseTorrent(String hash) throws DinosaurException;
	public native void ContinueTorrent(String hash) throws DinosaurException;
	public native void CheckTorrent(String hash) throws DinosaurException;
	public native void DeleteTorrent(String hash) throws DinosaurException;
	public native torrent_failure test() throws DinosaurException;
	public native torrent_failure[] test2();
	static
	{
		System.loadLibrary("dinosaur");
	}
}
