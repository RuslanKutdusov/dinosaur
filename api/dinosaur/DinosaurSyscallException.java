package dinosaur;

public class DinosaurSyscallException extends Throwable{

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public int errno;
	public DinosaurSyscallException(int errno) {
		this.errno = errno;
	}
}
