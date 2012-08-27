package dinosaur.info;

public class tracker {
	public enum TRACKER_STATUS
	{
		TRACKER_STATUS_OK,//Все ок
		TRACKER_STATUS_UPDATING,//обновляется
		TRACKER_STATUS_ERROR,//произошла ошибка
		TRACKER_STATUS_TIMEOUT,//таймаут запроса
		TRACKER_STATUS_UNRESOLVED,//неудалось разрезолвить доменное имя
		TRACKER_STATUS_INVALID_ANNOUNCE,//такой тип трекеров не поддерживаем
		TRACKER_STATUS_CONNECTING,//подключается к трекеру...
		TRACKER_STATUS_SEE_FAILURE_MESSAGE//произошла ошибка, для описания см. поле failure_mes
	};
	public String 			announce;//аннонс(url трекера)
	public TRACKER_STATUS 	status;//статус
	public String			failure_mes;//описание ошибки, есть только если status == TRACKER_STATUS_SEE_FAILURE_MESSAGE
	public long 			seeders;//кол-во сидов в раздаче
	public long 			leechers;//кол-во личеров в раздче
	public long				update_in;//обновление через n секунд
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
