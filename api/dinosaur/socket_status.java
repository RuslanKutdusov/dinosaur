package dinosaur;

public class socket_status {
	public boolean							listen;
	public int 								errno_;
	public DinosaurException.ERR_CODES 		exception_errcode;
	public socket_status() {
		// TODO Auto-generated constructor stub
	}
	public socket_status(boolean listen, int errno_, int exception_errcode)
	{
		this.listen = listen;
		this.errno_ = errno_;
		this.exception_errcode = DinosaurException.ERR_CODES.values()[exception_errcode];
	}
}
