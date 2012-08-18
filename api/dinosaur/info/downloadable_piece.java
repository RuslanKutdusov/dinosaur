package dinosaur.info;

import dinosaur.DOWNLOAD_PRIORITY;

public class downloadable_piece {
	public int 					index;
	public int					block2download;
	public int					downloaded_blocks;
	public DOWNLOAD_PRIORITY 	priority;
	public downloadable_piece()
	{
		
	}
	public downloadable_piece(int index, int block2download, int downloaded_blocks, int priority)
	{
		this.index = index;
		this.block2download = block2download;
		this.downloaded_blocks = downloaded_blocks;
		this.priority = DOWNLOAD_PRIORITY.values()[priority];
	}
}
