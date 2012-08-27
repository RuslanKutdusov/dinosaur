package dinosaur.info;

import dinosaur.DOWNLOAD_PRIORITY;

public class downloadable_piece {
	public long 				index;//индекс куска
	public long					block2download;//блоков к загрузке
	public long					downloaded_blocks;//блоков загружено
	public DOWNLOAD_PRIORITY 	priority;//приоритет загрузки
	public downloadable_piece()
	{
		
	}
	public downloadable_piece(long index, long block2download, long downloaded_blocks, int priority)
	{
		this.index = index;
		this.block2download = block2download;
		this.downloaded_blocks = downloaded_blocks;
		this.priority = DOWNLOAD_PRIORITY.values()[priority];
	}
}
