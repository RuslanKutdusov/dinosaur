package dinosaur.info;

public class peer {
	public String 		ip;
	public long 		downloaded;
	public long 		uploaded;
	public double 		downSpeed;
	public double 		upSpeed;
	public double 		available;
	public int			blocks2request;
	public int			requested_blocks;
	public boolean 		may_request;
	public peer()
	{
		
	}
	public peer(String ip, long downloaded, long uploaded, double downSpeed, double upSpeed, double available,
				int block2request, int requested_blocks, boolean may_request)
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
