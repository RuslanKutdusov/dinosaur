package dinosaur.info;

import dinosaur.DOWNLOAD_PRIORITY;

public class file_dyn {
	DOWNLOAD_PRIORITY	priority;//приоритет загрузки
	long				downloaded;//загружено байт
	public file_dyn()
	{
		
	}
	public file_dyn(int priority, long downloaded)
	{
		this.priority = DOWNLOAD_PRIORITY.values()[priority];
		this.downloaded = downloaded;
	}
}
