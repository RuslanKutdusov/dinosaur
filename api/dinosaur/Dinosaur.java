package dinosaur;

import dinosaur.info.*;

public class Dinosaur {
	public Dinosaur()
	{
		
	}
	/*
	 * Инициализации либы, первый вызов, который должен быть - этот
	 * если не удалось возобновить работу торрентов, вернет описание ошибки(причину) в списке
	 * для каждого из них
	 * Искючения:
	 * DinosaurException::ERR_CODE_CAN_NOT_CREATE_THREAD
	 * DinosaurSyscallException
	 */
	public native torrent_failure[] InitLibrary() throws DinosaurSyscallException;
	/*
	 * Последний вызов - этот, осбождает ресурсы, может занять время
	 */
	public native void ReleaseLibrary();
	/*
	 * чтобы добавить новый торрент - этот вызов для тебя, просит путь к торрент-файлу,
	 * возвращает объект Metafile, содержащий информацию из этого файла,
	 * которую необходимо отобразить юзеру, перед тем как он решит добавить торрент или нет
	 * Исключения:
	 * DinosaurSyscallException
	 * DinosaurException::ERR_CODE_NULL_REF          - пробросит если либа не инициализирована
	 * DinosaurException::ERR_CODE_INVALID_METAFILE  - корявый файл
	 */
	public native Metafile OpenMetafile(String metafile_path) throws DinosaurException, DinosaurSyscallException;
	/*
	 * если решит не добавлять - вызови, освободит маленько памяти(необязательно)
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF          - пробросит если либа не инициализирована
	 */
	public native void CloseMetafile() throws DinosaurException;
	/*
	 * если решит добавить, этот вызов добавит торрент, требует путь куда сохранить раздачу
	 * возвращает infohash в hex-e
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_UNDEF					- случилась какая та херь
	 * Exception::ERR_CODE_TORRENT_EXISTS			- такой торрент уже есть 
	 */
	public native String AddTorrent(String save_directory) throws DinosaurException;
	/*
	 * Приостанавливает торрент с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_OPERATION		- невозможно остановить торрент, может он уже остановлен?
	 */
	public native void PauseTorrent(String hash) throws DinosaurException;
	/*
	 * Возобновляет торрент с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_OPERATION		- невозможно возобновить торрент, может он уже остановлен?
	 */
	public native void ContinueTorrent(String hash) throws DinosaurException;
	/*
	 * Иницирует проверку целостности
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_OPERATION		- нельзя
	 */
	public native void CheckTorrent(String hash) throws DinosaurException;
	/*
	 * Удаляет торрент, with_data указывает удалять вместе с данными или нет
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native void DeleteTorrent(String hash, boolean with_data) throws DinosaurException;
	/*
	 * Возвращает статическую информацию о торренте с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native torrent_stat get_torrent_info_stat(String hash) throws DinosaurException;
	/*
	 * Возвращает динамическую информацию о торренте с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native torrent_dyn get_torrent_info_dyn(String hash) throws DinosaurException;
	/*
	 * Возвращает информацию о трекерах торрента с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native tracker[] get_torrent_info_trackers(String hash) throws DinosaurException;
	/*
	 * Возвращает статическую информацию о файлах торрента с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_FILE_INDEX		- неверный индекс. >= кол-ву файлов?
	 */
	public native file_stat get_torrent_info_file_stat(String hash, long index) throws DinosaurException;
	/*
	 * Возвращает динамическую информацию о файлах торрента с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_FILE_INDEX		- неверный индекс. >= кол-ву файлов?
	 */
	public native file_dyn get_torrent_info_file_dyn(String hash, long index) throws DinosaurException;
	/*
	 * Возвращает информацию о сидах торрента с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native peer[] get_torrent_info_seeders(String hash) throws DinosaurException;
	/*
	 * Возвращает информацию о личерах торрента с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native peer[] get_torrent_info_leechers(String hash) throws DinosaurException;
	/*
	 * Возвращает информацию о загружаемых кусках торрента с заданным хэшем
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native downloadable_piece[] get_torrent_info_downloadable_pieces(String hash) throws DinosaurException;
	/*
	 * Возвращает описание ошибки, которая приключилась с торрентом
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 */
	public native torrent_failure get_torrent_failure_desc(String hash) throws DinosaurException;
	/*
	 * Выставляет приоритет загрузки файла
	 * prio == 0 - низкий приоритет
	 * prio == 1 - нормальный
	 * prio == 2 - высокий
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_FILE_INDEX		- неверный индекс. >= кол-ву файлов?
	 * Exception::ERR_CODE_FAIL_SET_FILE_PRIORITY	- не получилось
	 * Exception::ERR_CODE_INVALID_OPERATION		- нельзя ставить приоритеты во время проверки
	 */
	public native void set_file_priority(String hash, long index, int prio)  throws DinosaurException;
	/*
	 * Выставляет приоритет загрузки файла
	 * Исключения:
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_TORRENT_NOT_EXISTS 		- торрента с таким хэшем нет
	 * Exception::ERR_CODE_INVALID_FILE_INDEX		- неверный индекс. >= кол-ву файлов?
	 * Exception::ERR_CODE_FAIL_SET_FILE_PRIORITY	- не получилось
	 * Exception::ERR_CODE_INVALID_OPERATION		- нельзя ставить приоритеты во время проверки
	 */
	public void set_file_priority(String hash, long index, DOWNLOAD_PRIORITY prio)  throws DinosaurException
	{
		set_file_priority(hash, index, prio.ordinal());
	}
	/*
	 * Возвращает список хэшей всех торрентов
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 */
	public native String[] get_TorrentList() throws DinosaurException;
	/*
	 * Возвращает список хэшей всех активных торрентов
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 */
	public native String[] get_active_torrents() throws DinosaurException;
	/*
	 * Возвращает список хэшей всех не активных торрентов, они в очереди
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 */
	public native String[] get_torrents_in_queue() throws DinosaurException;
	/*
	 * Возвращает состояние слушающего сокета
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 */
	public native socket_status get_socket_status() throws DinosaurException;
	/*
	 * После того как поменял конфиги, стоит вызвать, сохранит конфиги и применит их
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG		- не получилось сохранить конфиги
	 */
	public native void UpdateConfigs()  throws DinosaurException;
	/*
	 * Возвращает все конфиги в виде класса
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 */
	public native Configs get_configs() throws DinosaurException;
	/*
	 * методы ниже выставляют конфиги
	 */
	/*
	 * куда сохранять торренты по дефолту
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_DIR_NOT_EXISTS			- такой папки нет
	 * SyscallException
	 */
	public native void set_config_download_directory(String dir) throws DinosaurException, DinosaurSyscallException;
	
	/*
	 * какой порт слушать
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_PORT			- такой папки нет
	 */
	public native void set_config_port(long v) throws DinosaurException;
	
	/*
	 * размер кэша записи 
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_W_CACHE_SIZE
	 */
	public native void set_config_write_cache_size(long v) throws DinosaurException;
	
	/*
	 * размер кэша чтения
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_R_CACHE_SIZE	
	 */
	public native void set_config_read_cache_size(long v) throws DinosaurException;
	
	/*
	 * сколько макс. пиров просить у трекеров
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_TRACKER_NUM_WANT		
	 */
	public native void set_config_tracker_numwant(long v) throws DinosaurException;
	
	/*
	 * интервал обновления трекеров по дефолту
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_TRACKER_DEF_INTERVAL		
	 */
	public native void set_config_tracker_default_interval(long v) throws DinosaurException;
	
	/*
	 * макс. число сидов в каждом торренте
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_MAX_ACTIVE_SEEDS			
	 */
	public native void set_config_max_active_seeders(long v) throws DinosaurException;
	
	/*
	 * макс. число личеров в каждом торренте
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_MAX_ACTIVE_LEECHS			
	 */
	public native void set_config_max_active_leechers(long v) throws DinosaurException;
	
	/*
	 * отправлять  ли have-сообщения пирам
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 */
	public native void set_config_send_have(boolean v) throws DinosaurException;
	
	/*
	 * какой интерфес слушать
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_LISTEN_ON			
	 */
	public native void set_config_listen_on(String ip) throws DinosaurException;
	
	/*
	 * какой порт слушать
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS			
	 */
	public native void set_config_max_active_torrents(long v) throws DinosaurException;
	
	/*
	 * после какого ratio считать торрент завершенным
	 * DinosaurException::ERR_CODE_NULL_REF 		- пробросит если либа не инициализирована
	 * Exception::ERR_CODE_INVALID_FIN_RATIO			
	 */
	public native void set_config_fin_ratio(float v)  throws DinosaurException;
	static
	{
		System.loadLibrary("dinosaur");  	
	}
}
