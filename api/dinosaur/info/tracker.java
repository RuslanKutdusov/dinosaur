package dinosaur.info;

public class tracker {
	public enum TRACKER_STATUS
	{
		TRACKER_STATUS_OK,
		TRACKER_STATUS_UPDATING,
		TRACKER_STATUS_ERROR,
		TRACKER_STATUS_TIMEOUT,
		TRACKER_STATUS_UNRESOLVED,
		TRACKER_STATUS_INVALID_ANNOUNCE,
		TRACKER_STATUS_CONNECTING,
		TRACKER_STATUS_SEE_FAILURE_MESSAGE
	};
	public String 			announce;
	public TRACKER_STATUS 	status;
	public String			failure_mes;
	public long 			seeders;
	public long 			leechers;
	public long				update_in;
	public tracker()
	{
		
	}
	public tracker(String announce, int status, String failure_mes, long seeders, 
					long leechers, long update_in)
	{
		this.announce = announce;
		this.status = TRACKER_STATUS.values()[status];
		this.failure_mes = failure_mes;
		this.seeders = seeders;
		this.leechers = leechers;
		this.update_in = update_in;
	}
}
