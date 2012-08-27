package dinosaur.info;

public class peer {
	public String 		ip;//без комментариев
	public long 		downloaded;//загружено байт у него
	public long 		uploaded;//отдано байт ему
	public long	 		downSpeed;//скорость загрузки
	public long	 		upSpeed;//скорость отдачи
	public double 		available;//доступно у него(0.000 - 1.000)
	public long			blocks2request;//блоков к загрузке
	public long			requested_blocks;//запрошено блоков
	public boolean 		may_request;//можем ли мы у него запрашивать
	public peer()
	{
		
	}
	public peer(String ip, long downloaded, long uploaded, long downSpeed, 
			long upSpeed, double available,long block2request, long requested_blocks, 
			    boolean may_request)
	{
		this.ip = ip;
		this.downloaded = downloaded;
		this.uploaded = uploaded;
		this.downSpeed = downSpeed;
		this.upSpeed = upSpeed;
		this.available = available;
		this.blocks2request = block2request;
		this.requested_blocks = requested_blocks;
		this.may_request = may_request;
	}
}
