package dinosaur.info;

public class peer {
	public String 		ip;
	public long 		downloaded;
	public long 		uploaded;
	public long	 		downSpeed;
	public long	 		upSpeed;
	public double 		available;
	public long			blocks2request;
	public long			requested_blocks;
	public boolean 		may_request;
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
